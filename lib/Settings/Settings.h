#ifndef SETTINGS_H
#define SETTINGS_H
#include <FileSystem.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>
class Settings{
public:
  bool init(char* firmware);
  bool storeSettings(String s);
  bool loadSettings();
  String returnSettings();
  bool factoryReset();

  //Soft AP settings
  char apSSID[50];
  char apPass[50];

  int sensorType;

  bool buffer;

  unsigned long reportInterval;

private:
  FileSystem fileSystem;
  String discoveredNetworks;
  String loadedSettings;
  void setPublicVariables(JsonObject& settingsJSON);
  void parseBytes(const char* str, char sep, uint8_t* bytes, int maxBytes, int base);
  char* _firmware;
};
#endif
