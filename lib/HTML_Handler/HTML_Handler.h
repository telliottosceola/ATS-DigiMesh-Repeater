#ifndef HTML_HANDLER_H
#define HTML_HANDLER_H
#include <Settings.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>
// #include <AsyncElegantOTA.h>
class HTMLHandler{
public:
  void init(Settings &s);
  // void loop();
  void enterSoftAP();
  void exitSoftAP();

  bool requestPending = false;

  void stop();

  void registerSettingsUpdateCallback(void(*settingsUpdateCallback)(bool success));

  void updateStatusStream(String statusStream);

private:
  unsigned long commandReceiveTime = millis();

  Settings *settings;
  void onRequest(AsyncWebServerRequest *request);
  void (*_settingsUpdateCallback)(bool success);

  String statusString;
};
#endif
