#include <Arduino.h>
#include "RGBLED.h"
#include <esp32-hal-ledc.h>

// Constructor
void RGBLED::init(int rPin, int gPin, int bPin, common_type type) {
	redPin = rPin;
	greenPin = gPin;
	bluePin = bPin;
	commonType = type;
	ledcAttachPin(redPin, 1); // assign RGB led pins to channels
  ledcAttachPin(greenPin, 2);
	ledcAttachPin(bluePin, 3);
	ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
	ledcSetup(2, 12000, 8);
	ledcSetup(3, 12000, 8);
	turnOff();
	pinMode(buzzer, OUTPUT);
}

void RGBLED::writeRGB(int red, int green, int blue) {
  writeRed(red);
  writeGreen(green);
  writeBlue(blue);
}

void RGBLED::writeRed(int red) {
	redValue = red;
	redMappedValue = mapValue(redValue);
	ledcWrite(1,redMappedValue);
}

void RGBLED::writeGreen(int green) {
	greenValue = green;
	greenMappedValue = mapValue(greenValue);
	ledcWrite(2,greenMappedValue);
}

void RGBLED::writeBlue(int blue) {

	blueValue = blue;
	blueMappedValue = mapValue(blueValue);
	ledcWrite(3,blueMappedValue);
}

void RGBLED::turnOff() {
	writeRed(0);
	writeGreen(0);
	writeBlue(0);
}

void RGBLED::writeRandom() {
	int r = random(0,255);
	int g = random(0,255);
	int b = random(0,255);

	writeRGB(r,g,b);
}

int RGBLED::mapValue(int value) {
	value = (value < 0) ? 0 : (value > 255) ? 255 : value;
	value = (commonType == COMMON_ANODE) ? 255-value : value;
	return value;
}

// Convert a given HSV (Hue Saturation Value) to RGB(Red Green Blue) and set the led to the color
//   h is hue value, integer between 0 and 360
//   s is saturation value, double between 0 and 1
//   v is value, double between 0 and 1
// Stolen from: http://eduardofv.com/read_post/179-Arduino-RGB-LED-HSV-Color-Wheel-
// Based on: http://www.splinter.com.au/converting-hsv-to-rgb-colour-using-c/
void RGBLED::writeHSV(int h, double s, double v) {
  //this is the algorithm to convert from RGB to HSV
  double r=0;
  double g=0;
  double b=0;

  double hf=h/60.0;

  int i=(int)floor(h/60.0);
  double f = h/60.0 - i;
  double pv = v * (1 - s);
  double qv = v * (1 - s*f);
  double tv = v * (1 - s * (1 - f));

  switch (i)
  {
  case 0: //rojo dominante
    r = v;
    g = tv;
    b = pv;
    break;
  case 1: //verde
    r = qv;
    g = v;
    b = pv;
    break;
  case 2:
    r = pv;
    g = v;
    b = tv;
    break;
  case 3: //azul
    r = pv;
    g = qv;
    b = v;
    break;
  case 4:
    r = tv;
    g = pv;
    b = v;
    break;
  case 5: //rojo
    r = v;
    g = pv;
    b = qv;
    break;
  }

  //set each component to a integer value between 0 and 255
  int red=constrain((int)255*r,0,255);
  int green=constrain((int)255*g,0,255);
  int blue=constrain((int)255*b,0,255);

  writeRGB(red,green,blue);
}


// Cycle through the color wheel
// Stolen from: http://eduardofv.com/read_post/179-Arduino-RGB-LED-HSV-Color-Wheel-
void RGBLED::writeColorWheel(int dly) {
  //The Hue value will vary from 0 to 360, which represents degrees in the color wheel
  for(int hue=0;hue<360;hue++)
  {
    writeHSV(hue,1,1); //We are using Saturation and Value constant at 1
    delay(dly); //each color will be shown for 10 milliseconds
  }
}

void RGBLED::setMode(uint8_t MODE){
	if(MODE == MODE_DATA_RECEIVED){
		dataReceivedLED = true;
		dataReceivedTime = millis();
		writeRGB(255,165,0);
	}

	if(_MODE == MODE){
		return;
	}
	digitalWrite(buzzer,LOW);
	_MODE=MODE;
	previousTime = 0;
	flashIndex = 0;
	Serial.printf("RGB LED Mode Set to %i\n", MODE);
}

void RGBLED::setSignalStrength(uint8_t signalStrength){
	if(_signalStrength/2 != signalStrength){
		_signalStrength = signalStrength*2;
	}

}

void RGBLED::momentary(int red, int green, int blue, unsigned long duration){
	writeRed(red);
	writeBlue(blue);
	writeGreen(green);
	momentaryDuration = duration;
	momentaryStartTime = millis();
}

void RGBLED::loop(){

	if(momentaryDuration != 0){
		if(millis() >= momentaryStartTime+momentaryDuration){
			momentaryDuration = 0;
			writeRed(0);
			writeBlue(0);
			writeGreen(0);
		}else{
			return;
		}
	}

	if(dataReceivedLED && millis() < dataReceivedTime+minimumFlashTime){
		return;
	}else{
		dataReceivedLED = false;
	}

	switch(_MODE){
		case 0:{
			if(previousTime == 0){
				if(MODE_ERROR_MQTT_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_ERROR_MQTT_COLOR[0], MODE_ERROR_MQTT_COLOR[1], MODE_ERROR_MQTT_COLOR[2]);
				}
				previousTime = millis();

			}else{
				if(millis() > previousTime+MODE_ERROR_MQTT_SEQUENCE[flashIndex]){
					(flashIndex >= (MODE_ERROR_MQTT_SIZE-1))? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_ERROR_MQTT_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_ERROR_MQTT_COLOR[0], MODE_ERROR_MQTT_COLOR[1], MODE_ERROR_MQTT_COLOR[2]);
					}
				}
			}
			break;
		}
		case 1:{
			if(previousTime == 0){
				if(MODE_ERROR_I2C_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_ERROR_I2C_COLOR[0], MODE_ERROR_I2C_COLOR[1], MODE_ERROR_I2C_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_ERROR_I2C_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_ERROR_I2C_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_ERROR_I2C_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_ERROR_I2C_COLOR[0], MODE_ERROR_I2C_COLOR[1], MODE_ERROR_I2C_COLOR[2]);
					}
				}
			}
			break;
		}
		case 2:{
			if(previousTime == 0){
				if(MODE_ERROR_COMMS_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_ERROR_COMMS_COLOR[0], MODE_ERROR_COMMS_COLOR[1], MODE_ERROR_COMMS_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_ERROR_COMMS_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_ERROR_COMMS_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_ERROR_COMMS_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_ERROR_COMMS_COLOR[0], MODE_ERROR_COMMS_COLOR[1], MODE_ERROR_COMMS_COLOR[2]);
					}
				}
			}
			break;
		}
		case 3:{
			if(previousTime == 0){
				if(MODE_SIGNAL_STRENGTH_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_SIGNAL_STRENGTH_COLOR[0], MODE_SIGNAL_STRENGTH_COLOR[1], MODE_SIGNAL_STRENGTH_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_SIGNAL_STRENGTH_SEQUENCE[flashIndex]){
					(flashIndex >= _signalStrength-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_SIGNAL_STRENGTH_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_SIGNAL_STRENGTH_COLOR[0], MODE_SIGNAL_STRENGTH_COLOR[1], MODE_SIGNAL_STRENGTH_COLOR[2]);
					}
				}
			}
			break;
		}
		case 4:{
			if(previousTime == 0){
				if(MODE_BOOT_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_BOOT_COLOR[0], MODE_BOOT_COLOR[1], MODE_BOOT_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_BOOT_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_BOOT_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_BOOT_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_BOOT_COLOR[0], MODE_BOOT_COLOR[1], MODE_BOOT_COLOR[2]);
					}
				}
			}
			break;
		}
		case 6:{
			if(previousTime == 0){
				if(MODE_SETUP_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_SETUP_COLOR[0], MODE_SETUP_COLOR[1], MODE_SETUP_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_SETUP_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_SETUP_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_SETUP_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_SETUP_COLOR[0], MODE_SETUP_COLOR[1], MODE_SETUP_COLOR[2]);
					}
				}
			}
			break;
		}
		case 7:{
			if(previousTime == 0){
				if(MODE_WIFI_DISCONNECT_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_WIFI_DISCONNECT_COLOR[0], MODE_WIFI_DISCONNECT_COLOR[1], MODE_WIFI_DISCONNECT_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_WIFI_DISCONNECT_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_WIFI_DISCONNECT_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_WIFI_DISCONNECT_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_WIFI_DISCONNECT_COLOR[0], MODE_WIFI_DISCONNECT_COLOR[1], MODE_WIFI_DISCONNECT_COLOR[2]);
					}
				}
			}
			break;
		}
		case 8:{
			if(previousTime == 0){
				if(MODE_ALL_CLEAR_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_ALL_CLEAR_COLOR[0], MODE_ALL_CLEAR_COLOR[1], MODE_ALL_CLEAR_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_ALL_CLEAR_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_ALL_CLEAR_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_ALL_CLEAR_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_ALL_CLEAR_COLOR[0], MODE_ALL_CLEAR_COLOR[1], MODE_ALL_CLEAR_COLOR[2]);
					}
				}
			}
			break;
		}
		case 9:{
			writeRGB(0,255,0);
			break;
		}
		case 10:{
			writeRGB(255,165,0);
			break;
		}
		case 11:{
			if(previousTime == 0){
				writeRandom();
				previousTime = millis();
			}else{
				if(millis() > previousTime+200){
					previousTime = millis();
					writeRandom();
				}
			}
		}
		case 12:{
			writeRGB(0, 0, 0);
		}
		case 13:{
			if(previousTime == 0){
				if(MODE_SIGNAL_VERY_GOOD_INDEX_STATE[flashIndex] == 0){
					turnOff();
				}else{
					writeRGB(MODE_SIGNAL_VERY_GOOD_COLOR[0], MODE_SIGNAL_VERY_GOOD_COLOR[1], MODE_SIGNAL_VERY_GOOD_COLOR[2]);
				}
				previousTime = millis();
			}else{
				if(millis() > previousTime+MODE_SIGNAL_VERY_GOOD_SEQUENCE[flashIndex]){
					(flashIndex >= MODE_SIGNAL_VERY_GOOD_SIZE-1)? flashIndex = 0 : flashIndex++;
					previousTime = millis();
					if(MODE_SIGNAL_VERY_GOOD_INDEX_STATE[flashIndex] == 0){
						turnOff();
					}else{
						writeRGB(MODE_SIGNAL_VERY_GOOD_COLOR[0], MODE_SIGNAL_VERY_GOOD_COLOR[1], MODE_SIGNAL_VERY_GOOD_COLOR[2]);
					}
				}
			}
			break;
		}
	}
}
