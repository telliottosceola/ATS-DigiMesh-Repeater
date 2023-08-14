#include <GPIOHandler.h>

void GPIOHandler::init(Settings &s, RGBLED &rgb){
  settings = &s;
  rgbLED = &rgb;
  pinMode(button,INPUT_PULLUP);
}

bool buttonEnabled;
void GPIOHandler::loop(){
  if(digitalRead(button) == 0){
    if(!buttonPressed){
      buttonPressed = true;
      buttonPressTime = millis();
    }else{
      if(millis() > buttonPressTime+4000){
        _cfgButtonCallBack(millis() - buttonPressTime);
        buttonEnabled = false;
      }
    }
  }else{
    if(buttonPressed && buttonEnabled){
      _cfgButtonCallBack(millis() - buttonPressTime);
    }
    buttonEnabled = true;
    buttonPressed = false;
  }
}

bool GPIOHandler::checkCFGButton(){
  if(digitalRead(button) == 0){
    return true;
  }
  return false;
}

void GPIOHandler::registerButtonPressCallback(void(*cfgButtonCallBack)(unsigned long duration)){
  _cfgButtonCallBack = cfgButtonCallBack;
}
