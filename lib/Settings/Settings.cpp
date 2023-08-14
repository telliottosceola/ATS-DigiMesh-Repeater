#include <Settings.h>

// #define DEBUG

bool Settings::init(char* firmware){
  _firmware = firmware;
  if(!SPIFFS.begin(true)){
    #ifdef DEBUG
    Serial.println("SPIFFS Mount Failed");
    #endif
    return false;
  }else{
    return true;
  }
}

bool Settings::storeSettings(String s){
  DynamicJsonBuffer newSettingsBuffer;
  JsonObject& newSettingsJSON = newSettingsBuffer.parseObject(s);
  if(newSettingsJSON.success()){
    int storedSettingsLength = fileSystem.getFileSize(SPIFFS, "/settings.txt");
    if(storedSettingsLength != 0){
      char storedSettingsFileBuffer[storedSettingsLength+1];
      if(fileSystem.readFile(SPIFFS, "/settings.txt", storedSettingsFileBuffer, storedSettingsLength)){
        JsonObject& oldSettingsJSON = newSettingsBuffer.parseObject(String(storedSettingsFileBuffer));
        if(oldSettingsJSON.success()){
          //Get loaded settings into JSON
          for(auto kvp:newSettingsJSON){
            if(oldSettingsJSON.containsKey(kvp.key)){
              oldSettingsJSON[kvp.key] = kvp.value;
            }else{
              #ifdef DEBUG
              Serial.printf("invalid settings key: %s\n", kvp.key);
              #endif
            }
          }
          //set public variables based on new Settings
          setPublicVariables(oldSettingsJSON);

          File file = SPIFFS.open("/settings.txt", FILE_WRITE);
          if(file){
            oldSettingsJSON.printTo(file);
            file.close();
            return true;
          }else{
            #ifdef DEBUG
            Serial.println("failed to save settings to file in Settings.storeSettings");
            #endif
            return false;
          }

        }else{
          #ifdef DEBUG
          Serial.println("Invalid JSON in old settings");
          Serial.println(storedSettingsFileBuffer);
          #endif
          return false;
        }

      }else{
        #ifdef DEBUG
        Serial.println("readFile failed in Settings.storeSettings");
        #endif
        return false;
      }
    }else{
      #ifdef DEBUG
      Serial.println("settings.txt is empty in Settings.storeSettings");
      #endif
      return false;
    }
  }else{
    #ifdef DEBUG
    Serial.println("New Settings invalid JSON");
    Serial.println(s);
    #endif
    return false;
  }
}

bool Settings::loadSettings(){
  int storedSettingsLength = fileSystem.getFileSize(SPIFFS, "/settings.txt");
  if(storedSettingsLength != 0){
    char storedSettingsFileBuffer[storedSettingsLength+1];
    if(fileSystem.readFile(SPIFFS, "/settings.txt", storedSettingsFileBuffer, storedSettingsLength)){
      DynamicJsonBuffer jsonBuffer;
      JsonObject& storedSettingsJSON = jsonBuffer.parseObject(String(storedSettingsFileBuffer));
      if(storedSettingsJSON.success()){
        storedSettingsJSON.printTo(Serial);
        setPublicVariables(storedSettingsJSON);
        return true;
      }else{
        #ifdef DEBUG
        Serial.println("Invalid JSON in Settings.loadSettings");
        Serial.println(storedSettingsFileBuffer);
        #endif
        factoryReset();
        return false;
      }
    }else{
      #ifdef DEBUG
      Serial.println("readFile failed in Settings.loadSettings");
      #endif
      factoryReset();
      return false;
    }
  }else{
    #ifdef DEBUG
    Serial.println("settings.txt is empty in Settings.loadSettings");
    #endif
    factoryReset();
    return false;
  }
}

String Settings::returnSettings(){
  //Load Top Level Settings
  int storedSettingsLength = fileSystem.getFileSize(SPIFFS, "/settings.txt");
  if(storedSettingsLength != 0){
    char storedSettingsFileBuffer[storedSettingsLength+1];
    if(fileSystem.readFile(SPIFFS, "/settings.txt", storedSettingsFileBuffer, storedSettingsLength)){
      loadedSettings = String(storedSettingsFileBuffer);
    }
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject& storedSettingsJSON = jsonBuffer.parseObject(loadedSettings);
  if(storedSettingsJSON.success()){
    if(storedSettingsJSON.containsKey("ssid_password")){
      storedSettingsJSON["apPass"] = "__SET__";
    }
    storedSettingsJSON["mac"] = WiFi.macAddress();
    storedSettingsJSON["firmware"] = _firmware;
    String Settings;
    storedSettingsJSON.printTo(Settings);
    storedSettingsJSON.printTo(Serial);
    Serial.println();
    return Settings;
  }else{
    #ifdef DEBUG
    Serial.println("Parse JSON in stored settings failed in Settings.returnSettings");
    #endif
    return "";
  }

}

bool Settings::factoryReset(){
  #ifdef DEBUG
  Serial.println("factoryReset Settings");
  #endif
  int defaultSettingsLength = fileSystem.getFileSize(SPIFFS, "/settings_d.txt");
  if(defaultSettingsLength != 0){
    char defaultSettingsFileBuffer[defaultSettingsLength+1];
    if(fileSystem.readFile(SPIFFS, "/settings_d.txt", defaultSettingsFileBuffer, defaultSettingsLength)){
      DynamicJsonBuffer jsonBuffer;
      JsonObject& defaultSettingsJSON = jsonBuffer.parseObject(String(defaultSettingsFileBuffer));
      if(defaultSettingsJSON.success()){
        File file = SPIFFS.open("/settings.txt", FILE_WRITE);
        if(file){
          defaultSettingsJSON.printTo(file);
          file.close();
          return true;
        }
      }
    }
  }
  #ifdef DEBUG
  Serial.println("Default settings file is empty");
  #endif
  return false;
}

void Settings::setPublicVariables(JsonObject& settingsJSON){
  //set public variables based on new Settings
  memset(apSSID, 0, 50);
  strcpy(apSSID, settingsJSON["ap_ssid"]);
  memset(apPass, 0, 50);
  strcpy(apPass, settingsJSON["ap_password"]);

  sensorType = settingsJSON["sensor_type"].as<int>();

  buffer = settingsJSON["buffer"].as<bool>();

  reportInterval = settingsJSON["report_interval"].as<unsigned long>();
}
