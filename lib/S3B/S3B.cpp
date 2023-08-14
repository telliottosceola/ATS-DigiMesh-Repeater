#include "S3B.h"
// #define DEBUG

HardwareSerial serial1(2);

void S3B::init(Settings &s, bool atEnable, int baudRate){
	deviceSettings = &s;
	serial1.begin(baudRate);
	//Reset module settings in case ESP lost power/reset but XBee did not.
	char factoryResetID[3] = "FR";
	uint8_t commandData[0];
	atEnabled = atEnable;
	if(atEnable){
		atCommand(factoryResetID, commandData, 0);
	}

	#ifdef DEBUG
	Serial.println("module.init called");
	#endif
}

void S3B::reset(){
	pendingRequest = false;
}

bool S3B::handleSettings(){
	if(strcmp(localAddress, "") == 0){
		if(millis() > localAddressRequestTime+localAddressTimeout || localAddressRequestTime == 0){
			serial1.end();
			delay(100);
			serial1.begin(115200);
			sendATReadCommand(serialLowCommand, 0);
			localAddressRequestTime = millis();
			return false;
		}
	}else{
		if(strcmp(networkID, "") == 0){
			if(millis() > networkIDRequestTime+networkIDTimeout || networkIDRequestTime == 0){
				serial1.end();
				delay(100);
				serial1.begin(115200);
				sendATReadCommand(networkIDCommand, 2);
				networkIDRequestTime = millis();
				return false;
			}
		}else{
			if(strcmp(preambleID, "") == 0){
				if(millis() > preambleIDRequestTime+preambleIDTimeout || preambleIDRequestTime == 0){
					serial1.end();
					delay(100);
					serial1.begin(115200);
					sendATReadCommand(preambleCommand, 1);
					preambleIDRequestTime = millis();
					return false;
				}
			}else{
				if(strcmp(txPower, "") == 0){
					if(millis() > txPowerRequestTime+txPowerTimeout || txPowerRequestTime == 0){
						serial1.end();
						delay(100);
						serial1.begin(115200);
						sendATReadCommand(txPowerCommand, 3);
						txPowerRequestTime = millis();
						return false;
					}
				}else{
					return true;
				}
			}
		}
		return false;
	}
	return false;
}

int S3B::transmit(uint8_t *address, uint8_t *data, int len, int fID){
    int tlen = len+14;
    uint8_t payload[len+18];
    // len = len+14;

    uint8_t lsb = (tlen & 255);
    uint8_t msb = (tlen >> 8);

    // Serial.printf("MSB: %i \n", msb);
    // Serial.printf("LSB: %i \n", lsb);

    //build the packet
    payload[0] = startDelimiter;
    payload[1] = msb;
    payload[2] = lsb;
    payload[3] = frameType;
		if(frameID==0xFE){
			frameID = 1;
			payload[4] = frameID;
		}else{
			payload[4] = ++frameID;
		}

    //populate address in packet
    for(int i = 0; i < 8; i++){
        payload[5+i] = address[i];
    }
    payload[13] = reserved1;
    payload[14] = reserved2;
    payload[15] = bRadius;
    payload[16] = transmitOptions;
    //populate data in packet
    for(int i = 0; i < len; i++){
        payload[17+i] = data[i];
    }
    int c = 0;
    for(int i = 3; i < len+17; i++){
        c += payload[i];
    }
    uint8_t checksum = 0xFF - (c & 0xFF);
    payload[len+17] = checksum;
		loop();
		// Serial.print("Transmission Packet: ");
		// for(int i = 0; i < len+18; i++){
		// 	Serial.printf("%02X ",payload[i]);
		// }
		// Serial.println();
    serial1.write(payload, len+18);

		if(!deviceSettings->buffer){
			return frameID;
		}

		DynamicJsonBuffer jBuffer;
		JsonObject& transmissionsJSON = jBuffer.parseObject(transmissions);
		JsonObject& addressTransmissionsJSON = jBuffer.parseObject(addressTransmissions);
		char frameIDChar[4];
		memset(frameIDChar, 0, sizeof(frameIDChar));
		sprintf(frameIDChar, "%i", frameID);
		if(transmissionsJSON.containsKey(frameIDChar)){
			transmissionsJSON.remove(frameIDChar);
		}
		JsonObject& transmissionElement = transmissionsJSON.createNestedObject(frameIDChar);
		JsonArray& commandArray = transmissionElement.createNestedArray("command");
		JsonArray& destinationArray = transmissionElement.createNestedArray("destination");
		char addressChar[17];
		sprintf(addressChar,"%02X%02X%02X%02X%02X%02X%02X%02X",address[0],address[1],address[2],address[3],address[4],address[5],address[6],address[7]);
		commandArray.copyFrom(data, len);
		destinationArray.copyFrom(address, 8);
		transmissions = "";
		transmissionsJSON.printTo(transmissions);
		if(addressTransmissionsJSON.containsKey(addressChar)){
			int transmissionCount = addressTransmissionsJSON[addressChar].as<int>();
			addressTransmissionsJSON[addressChar] = transmissionCount++;
		}else{
			addressTransmissionsJSON[addressChar] = 1;
		}
		addressTransmissions = "";
		addressTransmissionsJSON.printTo(addressTransmissions);
		if(!pendingRequest){
			pendingRequest = true;
			lastComsTry = millis();
		}
		return frameID;
    // Serial.print("Sending: ");
    // for(int i = 0; i < len+18; i++){
    //     Serial.printf("%x, ", payload[i]);
    // }
    // Serial.println();
}

bool S3B::blockingTransmit(uint8_t *address, uint8_t *data, int len, uint8_t *buffer, int bufferSize, unsigned long receiveTimeout){
	int tlen = len+14;
	uint8_t payload[len+18];

	uint8_t lsb = (tlen & 255);
	uint8_t msb = (tlen >> 8);

	//build the packet
	payload[0] = startDelimiter;
	payload[1] = msb;
	payload[2] = lsb;
	payload[3] = frameType;
	if(frameID==0xFE){
		payload[4] = frameID;
		frameID = 0;
	}else{
		payload[4] = ++frameID;
	}

	//populate address in packet
	for(int i = 0; i < 8; i++){
		payload[5+i] = address[i];
	}
	payload[13] = reserved1;
	payload[14] = reserved2;
	payload[15] = bRadius;
	payload[16] = transmitOptions;
	//populate data in packet
	for(int i = 0; i < len; i++){
		payload[17+i] = data[i];
	}
	int c = 0;
	for(int i = 3; i < len+17; i++){
		c += payload[i];
	}
	uint8_t checksum = 0xFF - (c & 0xFF);
	payload[len+17] = checksum;
	loop();
	//Send the data
	serial1.write(payload, len+18);
	#ifdef DEBUG
	Serial.print("Blocking Transmit sent: ");
	for(int i = 0; i < len+18; i++){
		Serial.printf("%02X ", payload[i]);
	}
	Serial.println();
	#endif
	unsigned long startTime = millis();
	// bool ackReceived = false;
	while(millis() < startTime+receiveTimeout){
		while(serial1.available()){
			uint8_t startByte = 0;
			while(serial1.available()){
				startByte = serial1.read();
				if(startByte == startDelimiter){
					break;
				}
			}
			if(startByte != startDelimiter){
				//no valid data
				return false;
			}
			unsigned long start = millis();
			while(serial1.available() < 2 && millis() < start+ackTimeOut);
			if(serial1.available() < 2){
				return false;
			}
			uint8_t lenMSB = serial1.read();
			uint8_t lenLSB = serial1.read();
			int newDataLength = (lenMSB*256)+lenLSB;
			uint8_t receiveBuffer[newDataLength+4];
			receiveBuffer[0] = startByte;
			receiveBuffer[1] = lenMSB;
			receiveBuffer[2] = lenLSB;

			int count = 0;
			while(count != newDataLength+1 && millis() <= ackTimeOut+start){
				if(serial1.available() != 0){
					receiveBuffer[count+3] = serial1.read();
					count++;
				}
			}
			if(count < newDataLength+1){
				return false;
			}
			if(validateReceivedData(receiveBuffer, newDataLength+4)){
				// uint8_t frameType = receiveBuffer[3];
				if(receiveBuffer[3] == 0x90){
					uint8_t newData[newDataLength+4 - 16];
					memcpy(newData, receiveBuffer+15, sizeof(newData));
					if(newData[0] == 0x7C){
						memcpy(buffer, newData, bufferSize);
						return true;
					}else{
						#ifdef DEBUG
						Serial.println("Received packet payload header not 0x7C");
						Serial.print("Packet: ");
						for(int i = 0; i < bufferSize; i++){
							Serial.printf("%02X ",newData[i]);
						}
						Serial.println();
						#endif
					}
				}
			}else{
				#ifdef DEBUG
				Serial.println("Invalid data received in blocking transmit");
				delay(50);
				#endif
				return false;
			}
		}
	}
	#ifdef DEBUG
	Serial.println("No data returned in blocking transmit");
	#endif
	return false;
}

bool S3B::validateReceivedData(uint8_t *s3bData, int len){
    if(len < 4){
			#ifdef DEBUG
        Serial.println("Invalid packet, too short");
				#endif
        return false;
    }
		if(s3bData[0] != 0x7E){
			#ifdef DEBUG
			Serial.println("Invalid Header Byte");
			#endif
			return false;
		}
    int c = 0;
    for(int i = 3; i < len -1; i++){
        c += s3bData[i];
    }
    uint8_t checksum = 0xFF - (c & 0xFF);
    if(s3bData[len-1] == checksum){
        return true;
    }else{
			#ifdef DEBUG
        Serial.println("Invalid packet, checksum incorrect");
				Serial.print("Data:");
				for(int i = 0; i < len; i++){
					Serial.printf("%02X ",s3bData[i]);
				}
				#endif
        return false;
    }
}

int S3B::atCommand(char* commandID, uint8_t* commandData, int dataLength){
	int packetLen = dataLength+8;
	uint8_t command[8+dataLength];
	command[0] = 0x7E;
	command[1] = 0x00;
	command[2] = 4+dataLength;
	command[3] = 0x08;
	if(frameID >= 0xFE){
		frameID = 0;
		command[4] = frameID;
	}else{
		command[4] = ++frameID;
	}
	command[5] = commandID[0];
	command[6] = commandID[1];
	if(dataLength != 0){
		for(int i = 7; i < dataLength+7; i++){
			command[i] = commandData[i-7];
		}
	}
	int cc = 0;
	for(int i = 3; i < 7+dataLength; i++){
			cc += command[i];
	}
	uint8_t checksum = 0xFF - (cc & 0xFF);
	command[packetLen-1] = checksum;
	serial1.write(command, packetLen);
	// Serial.print("Sent to module: ");
	// for(int i = 0; i < packetLen; i++){
	// 	Serial.printf("%02X ", command[i]);
	// }
	// Serial.println();
	if(!pendingRequest){
		pendingRequest = true;
		lastComsTry = millis();
	}
	return command[4];
}

void S3B::sendATReadCommand(uint8_t* c, uint8_t fID){

	uint8_t command[8] = {0x7E, 0x00, 0x04, 0x08, fID, c[0], c[1], 0x00};
	#ifdef DEBUG
	Serial.print("sendATReadCommand: ");
	for(int i = 0; i < 8; i++){
		Serial.printf("%02X ", command[i]);
	}
	Serial.println();
	#endif
	if(fID != 0xFF){
		if(frameID >= 0xFE){
			frameID = 0;
			command[4] = frameID;
		}else{
			command[4] = ++frameID;
		}
	}
	int cc = 0;
	for(int i = 3; i < 7; i++){
			cc += command[i];
	}
	uint8_t checksum = 0xFF - (cc & 0xFF);
	command[7] = checksum;
	serial1.write(command, 8);
	if(!pendingRequest){
		pendingRequest = true;
		lastComsTry = millis();
	}
}

bool S3B::blockATCommand(uint8_t* packet, int packetLen){
	serial1.write(packet, packetLen);

	unsigned long startTime = millis();

	while(millis() < startTime+ackTimeOut){
		while(serial1.available()){
			uint8_t startByte = 0;
			while(serial1.available()){
				startByte = serial1.read();
				if(startByte == startDelimiter){
					break;
				}
			}
			if(startByte != startDelimiter){
				//no valid data
				return false;
			}
			unsigned long start = millis();
			while(serial1.available() < 2 && millis() < start+ackTimeOut);
			if(serial1.available() < 2){
				return false;
			}
			uint8_t lenMSB = serial1.read();
			uint8_t lenLSB = serial1.read();
			int newDataLength = (lenMSB*256)+lenLSB;
			uint8_t receiveBuffer[newDataLength+4];
			receiveBuffer[0] = startByte;
			receiveBuffer[1] = lenMSB;
			receiveBuffer[2] = lenLSB;

			int count = 0;
			while(count != newDataLength+1 && millis() <= ackTimeOut+start){
				if(serial1.available() != 0){
					receiveBuffer[count+3] = serial1.read();
					count++;
				}
			}
			if(count < newDataLength+1){
				return false;
			}
			if(validateReceivedData(receiveBuffer, newDataLength+4)){
				if(receiveBuffer[3] == 0x88 && receiveBuffer[4] == packet[4] && receiveBuffer[7] == 0x00){
					return true;
				}else{
					#ifdef DEBUG
					Serial.println("No ack received from AT command");
					#endif
					return false;
				}

			}else{
				#ifdef DEBUG
				Serial.println("Invalid data received in blocking transmit");
				#endif
				return false;
			}
		}
	}
	return false;
}

bool S3B::enterConfigMode(){
	//Set Network ID
	uint8_t fID;
	if(frameID >= 0xFE){
		frameID = 0;
		fID = frameID;
	}else{
		fID = ++frameID;
	}
	uint8_t setNetworkIDCommand[10] = {0x7E, 0x00, 0x06, 0x08, fID, 0x49, 0x44, 0x7B, 0xCD, 0x00};
	int cc = 0;
	for(int i = 3; i < 9; i++){
			cc += setNetworkIDCommand[i];
	}
	uint8_t checksum = 0xFF - (cc & 0xFF);
	setNetworkIDCommand[9] = checksum;
	if(!blockATCommand(setNetworkIDCommand, 10)){
		#ifdef DEBUG
		Serial.println("Failed to set network ID");
		Serial.print("Command sent: ");
		for(int i = 0; i < 10; i++){
			Serial.printf("%02X ", setNetworkIDCommand[i]);
		}
		Serial.println();
		#endif
		return false;
	}else{
		#ifdef DEBUG
		Serial.println("Network ID set");
		#endif
	}

	//Set encrpytion key
	if(frameID >= 0xFE){
		frameID = 0;
		fID = frameID;
	}else{
		fID = ++frameID;
	}
	uint8_t setEncryptionKey[24] = {0x7E, 0x00, 0x14, 0x08, fID, 0x4B, 0x59, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x00};
	cc = 0;
	for(int i = 3; i < 23; i++){
			cc += setEncryptionKey[i];
	}
	checksum = 0xFF - (cc & 0xFF);
	setEncryptionKey[23] = checksum;
	if(!blockATCommand(setEncryptionKey, 24)){
		#ifdef DEBUG
		Serial.println("Failed to set encryption key");
		Serial.print("Command sent: ");
		for(int i = 0; i < 24; i++){
			Serial.printf("%02X ", setEncryptionKey[i]);
		}
		Serial.println();
		#endif
		return false;
	}else{
		#ifdef DEBUG
		Serial.println("Encryption Key set");
		#endif
	}

	//Turn on Encryption
	if(frameID >= 0xFE){
		frameID = 0;
		fID = frameID;
	}else{
		fID = ++frameID;
	}
	uint8_t setEncryptionON[9] = {0x7E, 0x00, 0x05, 0x08, fID, 0x45, 0x45, 0x01, 0x00};
	cc = 0;
	for(int i = 3; i < 8; i++){
			cc += setEncryptionON[i];
	}
	checksum = 0xFF - (cc & 0xFF);
	setEncryptionON[8] = checksum;
	if(!blockATCommand(setEncryptionON, 9)){
		#ifdef DEBUG
		Serial.println("Failed to turn on encryption");
		Serial.print("Command sent: ");
		for(int i = 0; i < 9; i++){
			Serial.printf("%02X ", setEncryptionON[i]);
		}
		Serial.println();
		#endif
		return false;
	}else{
		#ifdef DEBUG
		Serial.println("Encryption turned on");
		#endif
	}
	return true;
}

bool S3B::exitConfigMode(){
	//Reset module
	uint8_t fID;
	if(frameID >= 0xFE){
		frameID = 0;
		fID = frameID;
	}else{
		fID = ++frameID;
	}
	uint8_t resetCommand[8] = {0x7E, 0x00, 0x04, 0x08, fID, 0x46, 0x52, 0x00};
	int cc = 0;
	for(int i = 3; i < 7; i++){
			cc += resetCommand[i];
	}
	uint8_t checksum = 0xFF - (cc & 0xFF);
	resetCommand[7] = checksum;
	if(!blockATCommand(resetCommand, 8)){
		return false;
	}
	return true;
}

void S3B::loop(){
	if(millis() - lastLoop > 1000){
		// float elapsed = (millis() - lastLoop)/1000.00;
		// #ifdef DEBUG
		// Serial.printf("Seconds since last loop: %0.2f\n", elapsed);
		// #endif
	}
	lastLoop = millis();
	if(boot && settingsLoaded == 4 && atEnabled){
		boot = false;
		_sLoadedCallback();
	}

	if(!serial1.available() && pendingRequest && millis() > lastComsTry+moduleResponseTimeout && millis() > lastReception+moduleResponseTimeout && atEnabled){
		serial1.end();
		delay(100);
		serial1.begin(115200);
		moduleReady = false;
		pendingRequest = false;
	}
	while(serial1.available()){
		uint8_t startByte = 0;
		while(serial1.available()){
			startByte = serial1.read();
			if(startByte == startDelimiter){
				break;
			}
		}
		if(startByte != startDelimiter){
			//no valid data
			return;
		}
		unsigned long start = millis();
		while(serial1.available() < 2 && millis() < start+ackTimeOut);
		if(serial1.available() < 2){
			return;
		}
		uint8_t lenMSB = serial1.read();
    uint8_t lenLSB = serial1.read();
    int newDataLength = (lenMSB*256)+lenLSB;
		uint8_t receiveBuffer[newDataLength+4];
		receiveBuffer[0] = startByte;
		receiveBuffer[1] = lenMSB;
		receiveBuffer[2] = lenLSB;

		int count = 0;
    while(count != newDataLength+1 && millis() <= ackTimeOut+start){
      if(serial1.available() != 0){
        receiveBuffer[count+3] = serial1.read();
        count++;
      }
    }
		if(count < newDataLength+1){
      return;
    }
		if(validateReceivedData(receiveBuffer, newDataLength+4)){
			moduleReady = true;
			pendingRequest = false;
			lastReception = millis();
			switch(receiveBuffer[3]){
				case 0x10:
				parseTransmitRequest(receiveBuffer, newDataLength+4);
				case 0x88:
				//AT Command Response
				parseATCommandResponse(receiveBuffer, newDataLength+4);
				break;
				case 0x8A:
				//Modem Status
				parseModemStatusResponse(receiveBuffer, newDataLength+4);
				break;
				case 0x8B:
				//Transmit Status
				parseTransmitStatus(receiveBuffer, newDataLength+4);
				break;
				case 0x8D:
				//Route information packet
				parseRouteInformationPacket(receiveBuffer, newDataLength+4);
				break;
				case 0x8E:
				//Aggregate Addressing Update frame
				parseAggregateAddressingUpdateFrame(receiveBuffer, newDataLength+4);
				break;
				case 0x90:
				//RX Indicator
				// Serial.println("Rx Indicator Packet");
				parseRecievedPacket(receiveBuffer, newDataLength+4);
				break;
				case 0x91:
				//Explicit Rx Indicator
				parseExplicitRecievedPacket(receiveBuffer, newDataLength+4);
				break;
				case 0x92:
				//Data Sample Rx Indicator frame
				parseDataSampleReceiveIndicator(receiveBuffer, newDataLength+4);
				break;
				case 0x95:
				//Node Identification Indicator
				parseNodeIdentificationIndicator(receiveBuffer, newDataLength+4);
				break;
				case 0x97:
				//Remote Command Response
				parseRemoteCommandResponse(receiveBuffer, newDataLength+4);
				break;
			}
		}
	}
	if(atEnabled){
		if(handleSettings()){
			if(millis() > lastComsTry+checkModuleInterval && settingsLoaded >= 4 && millis() > lastReception+checkModuleInterval){
				if(!pendingRequest){
					#ifdef DEBUG
					Serial.printf("Sending read serial low command %i\n",millis());
					#endif
					sendATReadCommand(serialLowCommand, 0);
					pendingRequest = true;
					lastComsTry = millis();
				}
			}
		}
	}
}

void S3B::parseTransmitRequest(uint8_t* data, size_t len){
	uint8_t newData[len-18];
	memcpy(newData, data+17, sizeof(newData));
	_sensorDataCallback(newData, sizeof(newData));
}

void S3B::parseATCommandResponse(uint8_t* data, size_t len){
	#ifdef DEBUG
	Serial.print("Parse AT Command Response:");
	for(int i = 0; i < len; i++){
		Serial.printf("%02X ", data[i]);
	}
	Serial.println();
	#endif

	_sATCallback(data, len);

	if(data[5] == 0x44 && data[6] == 0x42){
		//its an rssi request response
		if(data[7] != 0){
			// _rssiCallback(0);
		}else{
			_rssiCallback(data[8], rssiMac);
		}
	}
	if(data[5] == 0x53 && data[6] == 0x4C){
		//its a serial Low request response
		if(data[7] != 0){
			sprintf(localAddress, "00:00:00:00:00:00:FF:FF");
		}else{
			sprintf(localAddress, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", 0x00, 0x13, 0xA2, 0x00, data[8], data[9], data[10], data[11]);
			if(!addressLoaded){
				addressLoaded = true;
				settingsLoaded++;
			}

		}
		return;
	}
	if(data[5] == 0x44 && data[6] == 0x4E){
		if(data[7] != 0){
			#ifdef DEBUG
			Serial.println("DN command failed");
			#endif
		}else{
			#ifdef DEBUG
			Serial.print("DN returned: ");
			for(int i = 8; i < 28; i++){
				Serial.printf("%02X ", data[i]);
			}
			#endif
		}
	}
	if(data[5] == preambleCommand[0] && data[6] == preambleCommand[1]){
		char *format = (char*)"%i";
		if(data[7] != 0){
			#ifdef DEBUG
			Serial.println("Read module Preamble ID failed");
			#endif
			sprintf(preambleID, format, 0);
		}else{
			sprintf(preambleID, format, data[8]);
			#ifdef DEBUG
			Serial.printf("Read module Preamble ID: %s\n", preambleID);
			#endif
			settingsLoaded++;
		}
	}
	if(data[5] == networkIDCommand[0] && data[6] == networkIDCommand[1]){
		char *format = (char*)"%02X%02X";
		if(data[7] != 0){
			#ifdef DEBUG
			Serial.println("Read module Network ID failed");
			#endif
			sprintf(networkID, format, 0xFF, 0xFF);
		}else{
			sprintf(networkID, format, data[8],data[9]);
			#ifdef DEBUG
			Serial.printf("Read module Network ID: %s\n", networkID);
			#endif
			settingsLoaded++;
		}
	}
	if(data[5] == txPowerCommand[0] && data[6] == txPowerCommand[1]){
		char *format = (char*)"%i";
		if(data[7] != 0){
			#ifdef DEBUG
			Serial.println("Read module TX Power failed");
			#endif
			sprintf(txPower, format, 9);
		}else{
			sprintf(txPower, format, data[8]);
			#ifdef DEBUG
			Serial.printf("Read module TX Power: %s\n", txPower);
			#endif
			settingsLoaded++;
		}
	}
}

void S3B::parseModemStatusResponse(uint8_t* data, size_t len){

}

void S3B::parseTransmitStatus(uint8_t* data, size_t len){
	_tcallBack(data[8], data[4]);
}

void S3B::parseRouteInformationPacket(uint8_t* data, size_t len){

}

void S3B::parseAggregateAddressingUpdateFrame(uint8_t* data, size_t len){

}

void S3B::parseRecievedPacket(uint8_t* data, size_t len){
	uint8_t transmitterMac[8];
	memcpy(transmitterMac, data+4, 8);
	// Serial.print("Transmission from mating device: ");
	if(len < 17){
		return;
	}
	uint8_t newData[len - 16];
	memcpy(newData, data+15, sizeof(newData));
	_receiveCallback(newData, len - 16, transmitterMac);
	if(!serial1.available()){
		memcpy(rssiMac, transmitterMac, 8);
		sendATReadCommand(rssiCommand, 2);
	}
}

void S3B::parseExplicitRecievedPacket(uint8_t* data, size_t len){

}

void S3B::parseDataSampleReceiveIndicator(uint8_t* data, size_t len){

}

void S3B::parseNodeIdentificationIndicator(uint8_t* data, size_t len){

}

void S3B::parseRemoteCommandResponse(uint8_t* data, size_t len){

}

void S3B::receiveDataCallback(void (*receiveCallback)(uint8_t* data, size_t len, uint8_t* transmitterAddress)){
	_receiveCallback = receiveCallback;
}

void S3B::rssiUpdateCallback(void(*rssiCallback)(int rssi, uint8_t* address)){
	_rssiCallback = rssiCallback;
}

void S3B::readSerialLowCallback(void(*serialLowCallback)(uint8_t* localMac)){
	_serialLowCallback = serialLowCallback;
}

void S3B::transmissionStatusCallback(void(*tcallBack)(uint8_t status, uint8_t frameID)){
	_tcallBack = tcallBack;
}

void S3B::softwareATCallback(void(*sATCallback)(uint8_t*data, size_t len)){
	_sATCallback = sATCallback;
}

void S3B::settingsLoadedCallback(void(*sLoadedCallback)()){
	_sLoadedCallback = sLoadedCallback;
}

void S3B::sensorDataCallback(void(*sLoadedCallback)(uint8_t* data, size_t len)){
	_sensorDataCallback = sLoadedCallback;
}
