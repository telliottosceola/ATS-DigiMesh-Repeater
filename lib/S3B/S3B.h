#ifndef S3B_H
#define S3B_H
#include "HardwareSerial.h"
#include <Settings.h>
#include <ArduinoJson.h>
class S3B{
public:
	void init(Settings& s, bool atEnable = true, int baudRate = 115200);
	int transmit(uint8_t *address, uint8_t *data, int len, int fID = 0);
	bool blockingTransmit(uint8_t *address, uint8_t *data, int len, uint8_t *buffer, int bufferSize, unsigned long receiveTimeout);
	int parseReceive(uint8_t *s3bData, char *buffer, int len);
	bool validateReceivedData(uint8_t *s3bData, int len);
	int getReceiveDataLength(uint8_t *s3bData);
	bool parseAddress(String addr, uint8_t *buffer);
	int getRSSI();
	void loop();
	// void loadSettings();
	int atCommand(char* commandID, uint8_t* commandData, int dataLength);
	bool enterConfigMode();
	bool exitConfigMode();
	void reset();

	char localAddress[24] = "";
	unsigned long localAddressRequestTime = 0;
	unsigned long localAddressTimeout = 30000;
	char networkID[5] = "";
	unsigned long networkIDRequestTime = 0;
	unsigned long networkIDTimeout = 30000;
	char preambleID[2] = "";
	unsigned long preambleIDRequestTime = 0;
	unsigned long preambleIDTimeout = 30000;
	char txPower[2] = "";
	unsigned long txPowerRequestTime = 0;
	unsigned long txPowerTimeout = 30000;
	bool moduleReady = true;
	unsigned long lastComsTry = 0;
	unsigned long moduleResponseTimeout = 10000;
	bool pendingRequest = false;
	unsigned long checkModuleInterval = 30000;
	unsigned long lastReception;
	unsigned long checkReceiverInterval = 3000;

	unsigned long lastLoop = 0;

	String transmissions = "{}";
	String addressTransmissions = "{}";

	void receiveDataCallback(void (*receiveCallback)(uint8_t* data, size_t len, uint8_t* transmitterAddress));
	void rssiUpdateCallback(void(*rssiCallback)(int rssi, uint8_t* address));
	void readSerialLowCallback(void(*serialLowCallback)(uint8_t* localMac));
	void transmissionStatusCallback(void(*tcallBack)(uint8_t status, uint8_t frameID));
	void softwareATCallback(void(*sATCallback)(uint8_t*data, size_t len));
	void settingsLoadedCallback(void(*_sLoadedCallback)());
	void sensorDataCallback(void(*sLoadedCallback)(uint8_t* data, size_t len));

	//AT Commands
	uint8_t rssiCommand[2] = {0x44, 0x42};
	uint8_t serialLowCommand[2] = {'S', 'L'};
	uint8_t preambleCommand[2] = {'H','P'};
	uint8_t networkIDCommand[2] = {'I','D'};
	uint8_t txPowerCommand[2] = {'P','L'};
	uint8_t discoverNodeCommand[2] = {0x44, 0x4E};
	bool loaded = false;

private:
	bool addressLoaded = false;
	Settings* deviceSettings;
	void (*_receiveCallback)(uint8_t* data, size_t len, uint8_t* transmitterAddress);
	void (*_rssiCallback)(int rssi, uint8_t* address);
	void (*_serialLowCallback)(uint8_t* localMac);
	void (*_tcallBack)(uint8_t status, uint8_t frameID);
	void (*_sATCallback)(uint8_t*data, size_t len);
	void (*_sLoadedCallback)();
	void (*_sensorDataCallback)(uint8_t* data, size_t len);
	uint8_t startDelimiter = 0x7E;
	uint8_t frameType = 0x10;
	uint8_t frameID = 0x01;
	uint8_t reserved1 = 0xFF;
	uint8_t reserved2 = 0xFE;
	uint8_t bRadius = 0x00;
	uint8_t transmitOptions = 0x00;

	unsigned long ackTimeOut = 3000;
	uint8_t broadcastAddress[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF};
	bool atEnabled;

	// void flashLED(int count);
	void flushSerialPort();
	bool getAddress();
	void parseTransmitRequest(uint8_t* data, size_t len);
	void parseATCommandResponse(uint8_t* data, size_t len);
	void parseModemStatusResponse(uint8_t* data, size_t len);
	void parseTransmitStatus(uint8_t* data, size_t len);
	void parseRouteInformationPacket(uint8_t* data, size_t len);
	void parseAggregateAddressingUpdateFrame(uint8_t* data, size_t len);
	void parseRecievedPacket(uint8_t* data, size_t len);
	void parseExplicitRecievedPacket(uint8_t* data, size_t len);
	void parseDataSampleReceiveIndicator(uint8_t* data, size_t len);
	void parseNodeIdentificationIndicator(uint8_t* data, size_t len);
	void parseRemoteCommandResponse(uint8_t* data, size_t len);
	void sendATReadCommand(uint8_t* c, uint8_t fID);
	bool blockATCommand(uint8_t* packet, int packetLen);
	bool handleSettings();
	int settingsLoaded = 0;
	bool boot = true;
	uint8_t rssiMac[8];
};
#endif
