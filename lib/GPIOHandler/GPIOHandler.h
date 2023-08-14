#ifndef GPIOHANDLER_H
#define GPIOHANDLER_H
#include <Arduino.h>
#include <Settings.h>
#include <RGBLED.h>
class GPIOHandler{
public:
  void init(Settings &s, RGBLED &rgb);
  void loop();
  bool checkCFGButton();
  bool setupMode;
  void registerButtonPressCallback(void(*cfgButtonCallBack)(unsigned long duration));

private:
  Settings* settings;
  RGBLED* rgbLED;
  uint8_t button = 32;
  unsigned long buttonPressTime;
  bool buttonPressed;
  bool setupButtonTimeout = 3000;
  unsigned long factoryResetTimeout = 5000;
  bool factoryResetEnable = false;
  bool previousButtonState;
  int presses = 0;

  void (*_cfgButtonCallBack)(unsigned long duration);
};
#endif
