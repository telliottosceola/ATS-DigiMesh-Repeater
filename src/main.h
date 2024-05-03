#include <Arduino.h>
#include <GPIOHandler.h>
#include <RGBLED.h>
#include <HTML_Handler.h>
#include <Settings.h>
// #include <WiFiHandler.h>
#include <SPIFFS.h>
#include <S3B.h>
#include <NCDWireless.h>

GPIOHandler gpioHandler;
RGBLED rgbLED;
HTMLHandler httpHandler;
Settings settings;
// WiFiHandler wifiHandler;
S3B module;

void backgroundTasks(void* pvParameters);
void buttonPressCallback(unsigned long duration);
void settingsUpdateCallback(bool success);
void executeCommand(uint8_t* data, size_t len);

//S3B Module callbacks
void s3BReceiver(uint8_t* data, size_t len, uint8_t* transmitterAddress);
void s3BTransmitStatus(uint8_t status, uint8_t frameID);
void softwareS3BCommand();
void softwareATCommandResponse(uint8_t* data, size_t len);
void moduleSettingsLoaded();
void s3BRSSI(int rssi, uint8_t* transmitterAddress);
void parseBytes(const char* str, char sep, uint8_t* bytes, int maxBytes, int base);

TaskHandle_t backgroundTask;
bool setupMode = false;
bool previousSetupMode = false;

bool boot = true;

NCDWireless deviceDataParser;

int rssiPin = 21;

char* firmware = (char*)"1.0.0";

String devices = "{\"gateways\":{},\"repeaters\":{},\"sensors\":{}}";

unsigned long lastTransmit = 0;
uint8_t repeaterPacket[15] = {0x7F,0x00,0x00,0x00,0x00,0x00,0xFF,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t sensorPacket[15] = {0x7F,0x00,0x00,0x00,0x00,0x00,0xFF,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint8_t broadcastAddress[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF};
uint8_t transmitCount = 0;
bool sensorPacketReceived = false;
