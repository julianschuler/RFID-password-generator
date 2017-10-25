#include <UsbKeyboard.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define flatVersion					0 // set this to 1 when using the flat version
#define useWDT						0 // set this to 1 to enable the watch dog timer

#define passwordLength          	46
#define RFIDTagLength           	4
#define keyLength               	8
#define EEPROMSize              	512 //neccessary, getting it over EEPROM.length() won't work
#define maxPasswords            	(EEPROMSize-1-RFIDTagLength) / (passwordLength+RFIDTagLength)
#define buttonTimePressedLong   	3000
#define connectionTimeHost      	10000
#define buttonBit               	3
#define buttonRead()            	(PINA>>buttonBit&1)
#define buttonModeINPUT_PULLUP()	DDRA &= ~(1<<buttonBit); PORTA |= 1<<buttonBit

#if flatVersion
MFRC522 RFID(10, 3);
#else
MFRC522 RFID(10, 255);
#endif


const uint16_t randomKey = 54364;

uint8_t currentRFIDTag[RFIDTagLength];
uint8_t RFIDTagMaster[RFIDTagLength];
uint8_t RFIDTags[maxPasswords][RFIDTagLength];
uint8_t passwordCount;


void setup() {
	TIMSK0 = 0;
	usbDeviceDisconnect();
	delayMicroseconds(250000);
	usbDeviceConnect();
	buttonModeINPUT_PULLUP();
	randomSeed(randomKey);
	passwordCount = EEPROM.read(0);
	if (passwordCount != 255) {
		for (uint8_t i = 0; i < RFIDTagLength; i++) {
			RFIDTagMaster[i] = EEPROM.read(i + 1);
		}
	}
	for (uint8_t i = 0; i < passwordCount; i++) {
		for (uint8_t j = 0; j < RFIDTagLength; j++) {
			RFIDTags[i][j] = EEPROM.read(i * (passwordLength + RFIDTagLength) + j + RFIDTagLength + 1);
		}
	}
	//Serial.begin(9600);
	SPI.begin();
	RFID.PCD_Init();
	#if useWDT
		wdt_enable(WDTO_60MS);
	#endif
}


void loop() {
	if (RFID.PICC_IsNewCardPresent() && RFID.PICC_ReadCardSerial()) {
		UsbKeyboard.update();
		/*for (uint8_t j = 0; j < RFIDTagLength; j++) {
			currentRFIDTag[j] = RFID.uid.uidByte[j];
			}*/
		//Serial.print("Hex: ");
		//printHex(RFID.uid.uidByte, RFIDTagLength);
		bool knownRFIDTag = false;
		for (uint8_t j = 0; j < maxPasswords && !knownRFIDTag; j++) {
			knownRFIDTag = true;
			for (uint8_t k = 0; k < RFIDTagLength && knownRFIDTag; k++) {
				if (RFID.uid.uidByte[k] != RFIDTags[j][k]) {
					knownRFIDTag = false;
				}
			}
			if (knownRFIDTag) {
				if (buttonRead() == LOW) {
					changePassword(j);
				}
				else {
					sendPassword(j);
				}
			}
		}

		if (!knownRFIDTag && buttonRead() == LOW) {
			knownRFIDTag = true;
			for (uint8_t j = 0; j < RFIDTagLength && knownRFIDTag; j++) {
				if (RFID.uid.uidByte[j] != RFIDTagMaster[j]) {
					knownRFIDTag = false;
				}
			}
			if (knownRFIDTag) {
				for (uint16_t j = RFIDTagLength + 1; j < EEPROMSize; j++) {
					EEPROM.write(j, 255);											//delete all passwords and RFID tags
				}
				EEPROM.write(0, 255);
				memset(RFIDTags, 0, RFIDTagLength * maxPasswords);
			}
			else if (passwordCount == 255) {
				passwordCount = 0;
				Serial.println("Setze MasterTag");
				EEPROM.write(0, passwordCount);
				for (uint8_t j = 0; j < RFIDTagLength; j++) {
					RFIDTagMaster[j] = RFID.uid.uidByte[j];
					EEPROM.write(j + 1, RFIDTagMaster[j]);
					#if useWDT
						wdt_reset();
					#endif
				}
			}
			else {
				addPassword();
			}
		}
		RFID.PICC_HaltA();
		//cli();
		//disableINT0();
		for (uint16_t i = 0; i < 255; i++) {
			UsbKeyboard.update();
			delayMicroseconds(20000);
			#if useWDT
				wdt_reset();
			#endif
		}
	}
	UsbKeyboard.update();
	delayMicroseconds(20000);
	wdt_reset();
}


void sendPassword(uint8_t index) {
	uint8_t data;
	//Serial.println("send");
	for (uint8_t i = 0; i < passwordLength; i++) {
		data = EEPROM.read(index * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1);
		UsbKeyboard.sendKeyStroke(data % 36 + 4, (data / 36) * 2);
		//Serial.print(data, DEC);
		//Serial.print(' ');
		#if useWDT
			wdt_reset();
		#endif
	}
	//Serial.println("");
}


void changePassword(uint8_t index) {
	uint8_t data;
	for (uint8_t i = 0; i < passwordLength; i++) {
		data = random(72);
		EEPROM.write(index * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1, data);
		UsbKeyboard.sendKeyStroke(data % 36 + 4, (data / 36) * 2);
		//Serial.print(data, DEC);
		//Serial.print(' ');
		#if useWDT
			wdt_reset();
		#endif
	}
	//Serial.println("");
}


void addPassword() {
	//Serial.println("add");
	if (passwordCount < maxPasswords) {
		uint8_t data;
		for (uint8_t i = 0; i < RFIDTagLength; i++) {
			RFIDTags[passwordCount][i] = RFID.uid.uidByte[i];
			//Serial.print(passwordCount * (passwordLength+RFIDTagLength) + RFIDTagLength + i+1);
			//Serial.print(' ');
			EEPROM.write(passwordCount * (passwordLength + RFIDTagLength) + i + RFIDTagLength + 1, RFIDTags[passwordCount][i]);
			#if useWDT
				wdt_reset();
			#endif
		}
		//Serial.println("");
		for (uint8_t i = 0; i < passwordLength; i++) {
			data = random(72);
			EEPROM.write(passwordCount * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1, data);
			UsbKeyboard.sendKeyStroke(data % 36 + 4, (data / 36) * 2);
			//Serial.print(data, DEC);
			//Serial.print(' ');
			#if useWDT
			wdt_reset();
			#endif
		}
		//Serial.println("");
		passwordCount++;
		EEPROM.write(0, passwordCount);
	}
}

/*
void printHex(uint8_t *buffer, uint8_t bufferSize) {
	for (uint8_t i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
	Serial.println("");
}
*/