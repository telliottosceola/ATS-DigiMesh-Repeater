/*
	RGBLED.h - Library for controlling an RGB LED
	Created by Bret Stateham, November 13, 2014
	Released into the public domain
	You can get the latest version from http://github.com/BretStateham/rgbled

	To create an instance, call the RGBLED constructor with the red, green, and blue pins, as well as the type, COMMON_CATHODE or COMMON_ANODE

	There are two basic types of RGB leds, COMMON_CATHODE and COMMON_ANODE:
	COMMON_CATHODE RGB LEDs have a common cathode "leg" that should be connected to ground (GND) arduino
	COMMON_ANODE RGB LEDs have a common anode "leg" that should be connected to 5V on the arduino

	You need to physically connect the RGB LED's Red, Green and Blue pins to Pulse Width Modulation (PWM) capable pins on the Arduino
	On the Arduino, digital pins 3,5,6,9,10 & 11 are PWM pins as indicated by the "~" infront of their pin numbers on the board

	Create an RGBLED instance by passing the red, green, blue and type (COMMON_CATHODE or COMMON_ANODE) into the constructor.

	For example, the following code would create an RGBLED instance named rgbLED for a COMMON_ANODE RGB LEDs with the red, green and blue
	legs connected to 11,9 & 6 respectively:

	RGBLED rgbLed(11,9,6,COMMON_ANODE);

	You can then call then various methods on the RGBLED instance:

	writeRGB(red,green,blue);
	writeRed(red);
	writeGreen(green);
	writeBlue(blue);
	randomColor();
	turnOff();
	mapValue();

*/

#include <Arduino.h>

#ifndef RGBLED_h
#define RGBLED_h

typedef enum
{
  // Used to indicate the RGB LED is a Common Cathode LED
  COMMON_CATHODE = 0,
  // Used to indicate the RGB LED is a Common Anode LED
  COMMON_ANODE   = 1,
} common_type;

class RGBLED
{
public:
	//LED Pins
	int redPin;
	int greenPin;
	int bluePin;

	//RGB Values
	int redValue;
	int greenValue;
	int blueValue;

	//LED Type: COMMON_ANODE or COMMON_CATHODE
	common_type commonType;

	//RGB Values mapped based on commonType:
	int redMappedValue;
	int greenMappedValue;
	int blueMappedValue;

	//Constructor
	void init(int redPin,int greenPin, int bluePin, common_type commonType);

	// Single method to set all pins
	// red,green,blue values range from 0(off)-255(on)


	// Sets just the red pin
	// Values range from 0(off)-255(on)
	void writeRed(int red);

	// Sets just the greem pin
	// Values range from 0(off)-255(on)
	void writeGreen(int green);

	// Sets just the blue pin
	// Values range from 0(off)-255(on)
	void writeBlue(int blue);

	// Turns all pins off
	void turnOff(void);

	// Show a random color
	void writeRandom(void);

	// Writes the LED based on HSV values
	void writeHSV(int h, double s, double v);

	// Show the HSV Color Wheel
	// Take in the delay (dly) in milliseconds between each color
	void writeColorWheel(int dly);

	// Map a value based on the common_type
	int mapValue(int value);

  void setMode(uint8_t mode);
  void setSignalStrength(uint8_t signalStrength);

  void momentary(int red, int green, int blue, unsigned long duration);

  void loop();

  uint8_t MODE_ERROR_MQTT = 0;
  uint8_t MODE_ERROR_I2C = 1;
  uint8_t MODE_ERROR_COMMS = 2;
  uint8_t MODE_SIGNAL_STRENGTH = 3;
  uint8_t MODE_BOOT = 4;
  uint8_t MODE_WIFI_CONNECTED = 5;
  uint8_t MODE_SETUP = 6;
  uint8_t MODE_WIFI_DISCONNECTED = 7;
  uint8_t MODE_ALL_CLEAR = 8;
  uint8_t MODE_CLIENT_CONNECTED = 9;
  uint8_t MODE_DATA_RECEIVED = 10;
  uint8_t RANDOM = 11;
  uint8_t MODE_NONE = 12;
  uint8_t MODE_SIGNAL_VERY_GOOD = 13;

private:

  unsigned long minimumFlashTime = 100;
  unsigned long dataReceivedTime;
  bool dataReceivedLED = false;

  int buzzer = 33;
  void writeRGB(int red, int green, int blue);
  unsigned long pulseDuration = 200;
  unsigned long offDuration = 200;
  unsigned long cycleDelay = 1000;
  uint8_t _MODE;
  unsigned long previousTime;
  unsigned long flashIndex;
  uint8_t _signalStrength;
  unsigned long momentaryStartTime;
  unsigned long momentaryDuration;

  unsigned long MODE_ERROR_MQTT_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_ERROR_MQTT_COLOR[3] = {255,0,0};
  int MODE_ERROR_MQTT_INDEX_STATE[2] = {0, 1};
  int MODE_ERROR_MQTT_SIZE = 2;

  unsigned long MODE_ERROR_I2C_SEQUENCE[4] = {cycleDelay, pulseDuration, offDuration, pulseDuration}; //One Flash
  int MODE_ERROR_I2C_COLOR[3] = {255,0,0};
  int MODE_ERROR_I2C_INDEX_STATE[4] = {0, 1, 0, 1};
  int MODE_ERROR_I2C_SIZE = 4;

  unsigned long MODE_ERROR_COMMS_SEQUENCE[6] = {cycleDelay, pulseDuration, offDuration, pulseDuration, offDuration, pulseDuration}; //One Flash
  int MODE_ERROR_COMMS_COLOR[3] = {255,0,0};
  int MODE_ERROR_COMMS_INDEX_STATE[6] = {0, 1, 0, 1, 0, 1};
  int MODE_ERROR_COMMS_SIZE = 6;

  unsigned long MODE_SIGNAL_STRENGTH_SEQUENCE[10] = {cycleDelay, pulseDuration, offDuration, pulseDuration, offDuration, pulseDuration, offDuration, pulseDuration, offDuration, pulseDuration}; //One Flash
  int MODE_SIGNAL_STRENGTH_COLOR[3] = {0,255,0};
  int MODE_SIGNAL_STRENGTH_INDEX_STATE[10] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
  int MODE_SIGNAL_STRENGTH_SIZE = 10;

  unsigned long MODE_BOOT_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_BOOT_COLOR[3] = {255,255,0};
  int MODE_BOOT_INDEX_STATE[2] = {1, 0};
  int MODE_BOOT_SIZE = 2;

  unsigned long MODE_WIFI_CONNECTED_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_WIFI_CONNECTED_COLOR[3] = {255,255,255};
  int MODE_WIFI_CONNECTED_INDEX_STATE[2] = {0, 1};
  int MODE_WIFI_CONNECTED_SIZE = 2;

  unsigned long MODE_SETUP_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_SETUP_COLOR[3] = {0,0,255};
  int MODE_SETUP_INDEX_STATE[2] = {0, 1};
  int MODE_SETUP_SIZE = 2;

  unsigned long MODE_WIFI_DISCONNECT_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_WIFI_DISCONNECT_COLOR[3] = {255,255,255};
  int MODE_WIFI_DISCONNECT_INDEX_STATE[2] = {0, 1};
  int MODE_WIFI_DISCONNECT_SIZE = 2;

  unsigned long MODE_ALL_CLEAR_SEQUENCE[2] = {cycleDelay, pulseDuration}; //One Flash
  int MODE_ALL_CLEAR_COLOR[3] = {0,255,0};
  int MODE_ALL_CLEAR_INDEX_STATE[2] = {0, 1};
  int MODE_ALL_CLEAR_SIZE = 2;

  unsigned long MODE_SIGNAL_VERY_GOOD_SEQUENCE[4] = {cycleDelay, pulseDuration, offDuration, pulseDuration}; //One Flash
  int MODE_SIGNAL_VERY_GOOD_COLOR[3] = {0,255,0};
  int MODE_SIGNAL_VERY_GOOD_INDEX_STATE[4] = {0, 1, 0, 1};
  int MODE_SIGNAL_VERY_GOOD_SIZE = 4;
};

#endif
