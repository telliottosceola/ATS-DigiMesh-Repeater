#include <main.h>

void setup() {
  Serial.begin(115200);

  if(!SPIFFS.begin(true)){
    #ifdef DEBUG
    Serial.println("SPIFFS Mount Failed");
    #endif
  }
  settings.init(firmware);
  settings.loadSettings();

  rgbLED.init(2,15,13,COMMON_ANODE);
  rgbLED.setMode(rgbLED.MODE_WIFI_DISCONNECTED);

  gpioHandler.registerButtonPressCallback(buttonPressCallback);
  gpioHandler.init(settings, rgbLED);

  httpHandler.init(settings);
  httpHandler.registerSettingsUpdateCallback(settingsUpdateCallback);

  xTaskCreatePinnedToCore(backgroundTasks, "BackGround Tasks", 20000, NULL, 1, &backgroundTask, 1);

  // wifiHandler.init(settings, rgbLED);

  module.init(settings);
  module.receiveDataCallback(s3BReceiver);
  module.rssiUpdateCallback(s3BRSSI);
  module.transmissionStatusCallback(s3BTransmitStatus);
  module.softwareATCallback(softwareATCommandResponse);
  module.settingsLoadedCallback(moduleSettingsLoaded);
}

void loop() {
  if(previousSetupMode != setupMode){
    previousSetupMode = setupMode;
    #ifdef DEBUG
    Serial.printf("SetupMode: %s\n",setupMode?"True":"False");
    #endif
    if(setupMode){
      //Just entered setup mode
      rgbLED.setMode(rgbLED.MODE_SETUP);
      delay(1000);
      httpHandler.enterSoftAP();
    }else{
      //just exited setup mode
      // httpHandler.exitSoftAP();
    }
  }

  if(!setupMode){
    //Setup Mode Loop.
  }else{
    //Run Mode
    if(!module.moduleReady){
      rgbLED.setMode(rgbLED.MODE_ERROR_COMMS);
      return;
    }
  }

  module.loop();
  if(lastTransmit == 0 || millis() > lastTransmit+settings.reportInterval){
    module.transmit(broadcastAddress, repeaterPacket, 9);
    lastTransmit = millis();
  }
}

void buttonPressCallback(unsigned long duration){
  if(duration > 50){
    #ifdef DEBUG
    Serial.printf("Button pressed for %ims\n",(int)duration);
    #endif
  }
  if(!setupMode){
    setupMode = true;
    return;
  }else{
    if(duration <= 4000){
      setupMode = false;
      httpHandler.stop();
      rgbLED.setMode(rgbLED.MODE_WIFI_DISCONNECTED);
    }else{
      rgbLED.setMode(rgbLED.RANDOM);
      unsigned long start = millis();
      while(millis() < start+2000){
        rgbLED.loop();
      }
      settings.factoryReset();
      ESP.restart();
      httpHandler.stop();
      setupMode = false;
      rgbLED.setMode(rgbLED.MODE_WIFI_DISCONNECTED);
      ESP.restart();
    }
  }
}

void settingsUpdateCallback(bool success){
  if(success){
    ESP.restart();
  }
}

void backgroundTasks(void* pvParameters){
  for(;;){
    rgbLED.loop();
    gpioHandler.loop();
    vTaskDelay(10);
  }
  vTaskDelete( NULL );
}

void s3BReceiver(uint8_t* data, size_t len, uint8_t* transmitterAddress){
  // Serial.print("received data from address: ");
  // for(int i = 0; i < 8; i++){
  //   Serial.printf("%02x:", transmitterAddress[i]);
  // }
  // Serial.print(" Data: ");
  // for(int i = 0; i < len; i++){
  //   Serial.printf("%02x, ",data[i]);
  // }
  // Serial.println();

  char topic[25];
  sprintf(topic, "SN%02X%02X%02X%02X%02X%02X%02X%02X", transmitterAddress[0], transmitterAddress[1], transmitterAddress[2], transmitterAddress[3], transmitterAddress[4], transmitterAddress[5], transmitterAddress[6], transmitterAddress[7]);
  char sensorMac[23];
  sprintf(sensorMac, "%02X%02X%02X%02X%02X%02X%02X%02X", transmitterAddress[0], transmitterAddress[1], transmitterAddress[2], transmitterAddress[3], transmitterAddress[4], transmitterAddress[5], transmitterAddress[6], transmitterAddress[7]);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& mqttMessageObject = jsonBuffer.createObject();
  mqttMessageObject.createNestedObject("data");
  if(deviceDataParser.parseData(data, len, mqttMessageObject, false)){
    JsonObject& data = mqttMessageObject["data"].as<JsonObject&>();
      unsigned long pulseWidth = pulseIn(rssiPin, HIGH, 250);
      if(pulseWidth == 0){
        if(digitalRead(rssiPin) == HIGH){
          data["rssi"] = 100;
        }else{
          // Serial.println("Theoretically this should never print");
          data["rssi"] = 0;
        }
      }else{
        float highPercentage = (pulseWidth/208.00)*100.00;
        data["rssi"] = (int)highPercentage;
      }
      // mqttMessageObject.printTo(Serial);
      // Serial.println();

      JsonObject& knownDevices = jsonBuffer.parseObject(devices);
      if(mqttMessageObject["data"]["type"].as<int>() == 600){
        //This is a Gateway Device
        JsonObject& gateways = knownDevices["gateways"].as<JsonObject>();
        if(gateways.containsKey(sensorMac)){
          gateways[sensorMac]["rssi"] = data["rssi"];
        }else{
          JsonObject& newGateway = gateways.createNestedObject(sensorMac);
          newGateway["rssi"] = data["rssi"];
        }
        if(!setupMode){
          if(data["rssi"] >= 80){
            rgbLED.setMode(rgbLED.MODE_SIGNAL_VERY_GOOD);
          }
          if(data["rssi"] >= 60 && data["rssi"] < 80){
            rgbLED.setMode(rgbLED.MODE_ALL_CLEAR);
          }
          if(data["rssi"] >= 40 && data["rssi"] < 60){
            rgbLED.setMode(rgbLED.MODE_BOOT);
          }
          if(data["rssi"] >= 20 && data["rssi"] < 40){
            rgbLED.setMode(rgbLED.MODE_ERROR_MQTT);
          }
          if(data["rssi"] < 20){
            rgbLED.setMode(rgbLED.MODE_ERROR_I2C);
          }
        }

      }else{
        if(mqttMessageObject["data"]["type"].as<int>() == 601){
          //This is a Repeater Device
          JsonObject& repeaters = knownDevices["repeaters"].as<JsonObject>();
          if(repeaters.containsKey(sensorMac)){
            repeaters[sensorMac]["rssi"] = data["rssi"];
          }else{
            JsonObject& newRepeater = repeaters.createNestedObject(sensorMac);
            newRepeater["rssi"] = data["rssi"];
          }
        }else{
          //This is a sensor
          JsonObject& sensors = knownDevices["sensors"].as<JsonObject>();
          if(sensors.containsKey(sensorMac)){
            sensors[sensorMac]["rssi"] = data["rssi"];
          }else{
            JsonObject& newSensor = sensors.createNestedObject(sensorMac);
            newSensor["rssi"] = data["rssi"];
            newSensor["type"] = data["type"];
        }
      }
    }
    devices = "";
    knownDevices.printTo(devices);
    Serial.println(devices);
    httpHandler.updateStatusStream(devices);

    }else{
      Serial.println("Unknown device");
    }

}

void s3BTransmitStatus(uint8_t status, uint8_t frameID){
  // Serial.printf("Transmit Status: %i\n",status);
}

void softwareS3BCommand(){

}

void softwareATCommandResponse(uint8_t* data, size_t len){

}

void moduleSettingsLoaded(){

}

void s3BRSSI(int rssi, uint8_t* transmitterAddress){

}
