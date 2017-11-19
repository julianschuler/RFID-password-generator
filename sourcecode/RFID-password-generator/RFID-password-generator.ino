#include <USBKeyboard.h>
#include <EEPROM.h>
#include <avr/wdt.h>


#define flatVersion						0 // set this to 1 when using the flat version
#define printInformation				0 // set this to 1 to enable feedback for e. g. changing passwords
#define useWDT							0 // set this to 1 to enable the watch dog timer
#define Debug							1 // set this to 1 to enable Debug

#define EN 0
#define DE 1

#define Language						DE
#define maxPasswords					10

#if (Language == EN)
	#define Layout						LAYOUT_US
	#define MasterTagNotSetMsg			"Note: The master tag isn't set yet, press the button while holding a RFID tag above the reader to set this tag as master tag."
	#define SetMasterTagMsg				"The master tag was set."
	#define DeletedAllDataMsg			"All data has been deleted."
	#define PressButtonMsg				"To delete all data, press the button and hold the master tag above the reader again."
	#define ChangePasswordMsg1			"Changed password for RFID tag #"
	#define ChangePasswordMsg2			'.'
	#define AddRFIDTagMsg1				"Added new RFID tag as RFID tag #"
	#define AddRFIDTagMsg2				'.'
#elif (Language == DE)
	#define Layout						LAYOUT_DE
	#define MasterTagNotSetMsg			"Hinweis: Der Master-Tag wurde noch nicht festgelegt, druecke den Taster und halte waehrenddessen einen RFID-Tag ueber den Leser, um diesen als Master-Tag zu verwenden."
	#define SetMasterTagMsg				"Der Master-Tag wurde festgelegt."
	#define DeletedAllDataMsg			"Alle Daten wurden geloescht."
	#define PressButtonMsg				"Um alle Daten zu loeschen, druecke den Taster und halte den Master-Tag erneut ueber den Leser."
	#define ChangePasswordMsg1			"Das Passwort fuer RFID-Tag #"
	#define ChangePasswordMsg2			" wurde geaendert."
	#define AddRFIDTagMsg1				"Ein neuer RFID-Tag wurde als RFID-Tag #"
	#define AddRFIDTagMsg2				" hinzugefuegt."
#endif


#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
	#include <SoftwareSerial.h>
	
	#define TINY_SPI
	#define SPI_begin					TinySPI_begin
	#define SPI_transfer				TinySPI_transfer
	#define SPI_beginTransaction		uint8_t // empty
	#define SPI_endTransaction			uint8_t // empty
	
	#define buttonBit					3
	#define buttonPort					A
	
	#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
		#define CS						4	// PA4
		#define DI						0	// PA0
		#define DO						1	// PA1
		#define USCK					2	// PA2
	#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
		#define CS						7	// PA3
		#define DI						4	// PA6
		#define DO						5	// PA5
		#define USCK					6	// PA4
	#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__)
		#define CS						13	// PB4
		#define DI						14	// PB5
		#define DO						15	// PB6
		#define USCK					16	// PB7
	#endif
	
	#if Debug
		// connect D3 to the RX pin of an UART interface (e. g. an Arduino with Reset connected to GND), leave TX not connected
		SoftwareSerial mSerial(255, 3);
	#endif
#else
	#include <SPI.h>
	
	#define SPI_begin					SPI.begin
	#define SPI_transfer				SPI.transfer
	#define SPI_beginTransaction		SPI.beginTransaction
	#define SPI_endTransaction			SPI.endTransaction
	#define mSerial						Serial
	#define CS							9
	
	#define buttonBit					5
	#define buttonPort					D
	
	SPISettings spiSettings = SPISettings(SPI_CLOCK_DIV4, MSBFIRST, SPI_MODE0);
#endif

#define usbDeviceConnect()
#define usbDeviceDisconnect()

#define conc(a, b)						(a ## b)
#define concat(a, b)					conc(a, b) //double define allows concatenante strings defined by macros

#define RFIDTagLength					4
#define EEPROMSize						512
#define passwordLength					(EEPROMSize-1-RFIDTagLength) / maxPasswords - RFIDTagLength
#define buttonRead()					(concat(PIN, buttonPort)>>buttonBit & 1)
#define buttonModeINPUT_PULLUP()		(concat(DDR, buttonPort)) &= ~(1<<buttonBit); (concat(PORT, buttonPort)) |= 1<<buttonBit

#define PICC_CMD_REQA					0x26
#define PICC_CMD_HLTA					0x50
#define PICC_CMD_CT						0x88
#define PICC_CMD_SEL_CL1				0x93

#define PCD_Idle						0x00
#define PCD_CalcCRC						0x03
#define PCD_Transceive					0x0C
#define PCD_SoftReset					0x0F

#define CommandReg						0x01 << 1
#define ComIrqReg						0x04 << 1
#define DivIrqReg						0x05 << 1
#define ErrorReg						0x06 << 1
#define FIFODataReg						0x09 << 1
#define FIFOLevelReg					0x0A << 1
#define ControlReg						0x0C << 1
#define BitFramingReg					0x0D << 1
#define CollReg							0x0E << 1
#define ModeReg							0x11 << 1
#define TxModeReg						0x12 << 1
#define RxModeReg						0x13 << 1
#define TxControlReg					0x14 << 1
#define TxASKReg						0x15 << 1
#define CRCResultRegH					0x21 << 1
#define CRCResultRegL					0x22 << 1
#define ModWidthReg						0x24 << 1
#define TModeReg						0x2A << 1
#define TPrescalerReg					0x2B << 1
#define TReloadRegH						0x2C << 1
#define TReloadRegL						0x2D << 1


USBKeyboard mKeyboard = USBKeyboard(Layout);

const uint32_t randomKey = 987256;

uint8_t RFIDTagMaster[RFIDTagLength];
uint8_t passwordCount;

uint8_t uid[4];


void setup() {
	TIMSK0 = 0;
	buttonModeINPUT_PULLUP();
	randomSeed(randomKey);
	passwordCount = EEPROM.read(0);
	if (passwordCount != 255) {
		for (uint8_t i = 0; i < RFIDTagLength; i++) {
			RFIDTagMaster[i] = EEPROM.read(i + 1);
		}
	}
	#if printInformation
	else {
		mKeyboard.println(MasterTagNotSetMsg);
	}
	#endif
	SPI_begin();
	PCD_Init();
	#if Debug
	mSerial.begin(9600);
	#endif
	#if useWDT
	wdt_enable(WDTO_60MS);
	#endif
}


void loop() {
	if (PICC_IsNewCardPresent() && PICC_ReadCardSerial()) {
		PICC_HaltA();
		#if Debug
		mSerial.print("Found new tag: ");
		for (uint8_t i = 0; i < 4; i++) {
			mSerial.print(uid[i], HEX);
			mSerial.print(' ');
		}
		mSerial.println();
		#endif
		if (passwordCount == 255) {
			if (buttonRead() == LOW) {
				passwordCount = 0;
				#if Debug
				mSerial.println("Set master tag");
				#endif
				EEPROM.write(0, passwordCount);
				for (uint8_t i = 0; i < RFIDTagLength; i++) {
					RFIDTagMaster[i] = uid[i];
					EEPROM.write(i + 1, RFIDTagMaster[i]);
					#if useWDT
					wdt_reset();
					#endif
				}
				#if printInformation
				mKeyboard.println(SetMasterTagMsg);
				#endif
			}
		}
		else {
			bool knownRFIDTag = false;
			for (uint8_t i = 0; i < RFIDTagLength && !knownRFIDTag; i++) {
				knownRFIDTag = true;
				if (uid[i] != RFIDTagMaster[i]) {
					knownRFIDTag = false;
				}
			}
			if (knownRFIDTag) {
				if (buttonRead() == LOW) {
					for (uint16_t i = RFIDTagLength + 1; i < EEPROMSize; i++) {
						EEPROM.write(i, 255);
					}
					EEPROM.write(0, 255);
					#if printInformation
					mKeyboard.println(DeletedAllDataMsg);
					#endif
				}
				#if printInformation
				else {
					mKeyboard.println(PressButtonMsg);
				}
				#endif
			}
			for (uint8_t i = 0; i < maxPasswords && !knownRFIDTag; i++) {
				knownRFIDTag = true;
				for (uint8_t j = 0; j < RFIDTagLength && knownRFIDTag; j++) {
					if (uid[j] != EEPROM.read(i * (passwordLength + RFIDTagLength) + j + RFIDTagLength + 1)) {
						knownRFIDTag = false;
					}
				}
				if (knownRFIDTag) {
					if (buttonRead() == LOW) {
						ChangePassword(i);
					}
					else {
						SendPassword(i);
					}
				}
			}
			
			if (!knownRFIDTag && buttonRead() == LOW) {
				AddRFIDTag();
			}
		}
		for (uint16_t i = 0; i < 255; i++) {
			mKeyboard.update();
			delayMicroseconds(20000);
			#if useWDT
			wdt_reset();
			#endif
		}
	}
	mKeyboard.update();
	delayMicroseconds(20000);
	#if useWDT
	wdt_reset();
	#endif
}


void SendPassword(uint8_t index) {
	uint8_t data;
	#if Debug
	mSerial.println("Sending password");
	#endif
	for (uint8_t i = 0; i < passwordLength; i++) {
		data = EEPROM.read(index * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1);
		mKeyboard.sendKey(data % 36 + 4, (data / 36) * 2);
		#if useWDT
		wdt_reset();
		#endif
	}
}


void ChangePassword(uint8_t index) {
	uint8_t data;
	#if Debug
	mSerial.println("Changing password");
	#endif
	for (uint8_t i = 0; i < passwordLength; i++) {
		data = random(72);
		EEPROM.write(index * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1, data);
		#if useWDT
		wdt_reset();
		#endif
	}
	#if printInformation
	mKeyboard.print(ChangePasswordMsg1);
	mKeyboard.print(index + 1);
	mKeyboard.println(ChangePasswordMsg2);
	#endif
}


void AddRFIDTag() {
	#if Debug
	mSerial.println("Adding RFID tag");
	#endif
	if (passwordCount < maxPasswords) {
		uint8_t data;
		for (uint8_t i = 0; i < RFIDTagLength; i++) {
			EEPROM.write(passwordCount * (passwordLength + RFIDTagLength) + i + RFIDTagLength + 1, uid[i]);
			#if useWDT
			wdt_reset();
			#endif
		}
		for (uint8_t i = 0; i < passwordLength; i++) {
			data = random(72);
			EEPROM.write(passwordCount * (passwordLength + RFIDTagLength) + i + 2*RFIDTagLength + 1, data);
			#if useWDT
			wdt_reset();
			#endif
		}
		passwordCount++;
		EEPROM.write(0, passwordCount);
	}
	#if printInformation
	mKeyboard.print(AddRFIDTagMsg1);
	mKeyboard.print(passwordCount);
	mKeyboard.println(AddRFIDTagMsg2);
	#endif
}


#ifdef TINY_SPI
void TinySPI_begin() {
	pinMode(USCK, OUTPUT);
	pinMode(DO, OUTPUT);
	pinMode(DI, INPUT);
	USICR = _BV(USIWM0);
}


uint8_t TinySPI_transfer(uint8_t b) {
	USIDR = b;
	USISR = _BV(USIOIF);
	do
		USICR = _BV(USIWM0) | _BV(USICS1) | _BV(USICLK) | _BV(USITC);
	while ((USISR & _BV(USIOIF)) == 0);
	return USIDR;
}
#endif


void PCD_WriteRegister(uint8_t reg, uint8_t value) {
	SPI_beginTransaction(spiSettings);
	digitalWrite(CS, LOW);
	SPI_transfer(reg);
	SPI_transfer(value);
	digitalWrite(CS, HIGH);
	SPI_endTransaction();
}


void PCD_WriteRegister(uint8_t reg, uint8_t count, uint8_t *values) {
	SPI_beginTransaction(spiSettings);
	digitalWrite(CS, LOW);
	SPI_transfer(reg);
	for (uint8_t index = 0; index < count; index++) {
		SPI_transfer(values[index]);
	}
	digitalWrite(CS, HIGH);
	SPI_endTransaction();
}


uint8_t PCD_ReadRegister(uint8_t reg) {
	uint8_t value;
	SPI_beginTransaction(spiSettings);
	digitalWrite(CS, LOW);
	SPI_transfer(0x80 | reg);
	value = SPI_transfer(0);
	digitalWrite(CS, HIGH);
	SPI_endTransaction();
	return value;
}


void PCD_ReadRegister(uint8_t reg, uint8_t count, uint8_t *values, uint8_t rxAlign) {
	if (count == 0)
		return;
	uint8_t address = 0x80 | reg;
	uint8_t index = 0;
	SPI_beginTransaction(spiSettings);
	digitalWrite(CS, LOW);
	count--;
	SPI_transfer(address);
	if (rxAlign) {
		uint8_t mask = (0xFF << rxAlign) & 0xFF;
		uint8_t value = SPI_transfer(address);
		values[0] = (values[0] & ~mask) | (value & mask);
		index++;
	}
	while (index < count) {
		values[index] = SPI_transfer(address);
		index++;
	}
	values[index] = SPI_transfer(0);
	digitalWrite(CS, HIGH);
	SPI_endTransaction();
}


void PCD_Init() {
	pinMode(CS, OUTPUT);
	digitalWrite(CS, HIGH);
	
	PCD_WriteRegister(CommandReg, PCD_SoftReset);
	delayMicroseconds(50000);
	while (PCD_ReadRegister(CommandReg) & 0x10) {}
	
	PCD_WriteRegister(TxModeReg, 0x00);
	PCD_WriteRegister(RxModeReg, 0x00);
	PCD_WriteRegister(ModWidthReg, 0x26);
	PCD_WriteRegister(TModeReg, 0x80);
	PCD_WriteRegister(TPrescalerReg, 0xA9);
	PCD_WriteRegister(TReloadRegH, 0x03);
	PCD_WriteRegister(TReloadRegL, 0xE8);
	PCD_WriteRegister(TxASKReg, 0x40);
	PCD_WriteRegister(ModeReg, 0x3D);
	
	uint8_t value = PCD_ReadRegister(TxControlReg);
	if ((value & 0x03) != 0x03)
		PCD_WriteRegister(TxControlReg, value | 0x03);
}


bool PCD_CalculateCRC(uint8_t *data, uint8_t length, uint8_t *result) {
	PCD_WriteRegister(CommandReg, PCD_Idle);
	PCD_WriteRegister(DivIrqReg, 0x04);
	PCD_WriteRegister(FIFOLevelReg, 0x80);
	PCD_WriteRegister(FIFODataReg, length, data);
	PCD_WriteRegister(CommandReg, PCD_CalcCRC);
	
	for (uint16_t i = 5000; i > 0; i--) {
		uint8_t n = PCD_ReadRegister(DivIrqReg);
		if (n & 0x04) {
			PCD_WriteRegister(CommandReg, PCD_Idle);
			result[0] = PCD_ReadRegister(CRCResultRegL);
			result[1] = PCD_ReadRegister(CRCResultRegH);
			return true;
		}
	}
	return false;
}


uint8_t PCD_CommunicateWithPICC(uint8_t *sendData, uint8_t sendLen, uint8_t *backData = nullptr, uint8_t *backLen = nullptr, uint8_t *validBits = nullptr, uint8_t rxAlign = 0) {
	uint8_t txLastBits = validBits ? *validBits : 0;
	uint8_t bitFraming = (rxAlign << 4) + txLastBits;
	
	PCD_WriteRegister(CommandReg, PCD_Idle);
	PCD_WriteRegister(ComIrqReg, 0x7F);
	PCD_WriteRegister(FIFOLevelReg, 0x80);
	PCD_WriteRegister(FIFODataReg, sendLen, sendData);
	PCD_WriteRegister(BitFramingReg, bitFraming);
	PCD_WriteRegister(CommandReg, PCD_Transceive);
	PCD_WriteRegister(BitFramingReg, PCD_ReadRegister(BitFramingReg) | 0x80);
	
	uint16_t i;
	for (i = 2000; i > 0; i--) {
		uint8_t n = PCD_ReadRegister(ComIrqReg);
		if (n & 0x30)
			break;
		if (n & 0x01)
			return 0;
	}
	
	if (i == 0)
		return 0;
	uint8_t errorRegValue = PCD_ReadRegister(ErrorReg);
	if (errorRegValue & 0x13)
		return 0;
	if (backData && backLen) {
		uint8_t n = PCD_ReadRegister(FIFOLevelReg);
		if (n > *backLen)
			return 0;
		*backLen = n;
		PCD_ReadRegister(FIFODataReg, n, backData, rxAlign);
		if (validBits)
			*validBits = PCD_ReadRegister(ControlReg) & 0x07;
	}
	
	if (errorRegValue & 0x08)
		return 2;
	return 1;
}


bool PICC_IsNewCardPresent() {
	uint8_t bufferATQA[2];
	uint8_t bufferSize = sizeof(bufferATQA);
	uint8_t validBits = 7;
	uint8_t cmd = PICC_CMD_REQA;
	
	PCD_WriteRegister(TxModeReg, 0x00);
	PCD_WriteRegister(RxModeReg, 0x00);
	PCD_WriteRegister(ModWidthReg, 0x26);
	PCD_WriteRegister(CollReg, PCD_ReadRegister(CollReg) & 0x7F);
	
	return PCD_CommunicateWithPICC(&cmd, 1, bufferATQA, &bufferSize, &validBits);
}


bool PICC_ReadCardSerial() {
	uint8_t validBits = 0;
	bool selectDone = false;
	bool useCascadeTag;
	uint8_t cascadeLevel = 1;
	uint8_t count;
	uint8_t index;
	uint8_t uidIndex;
	int8_t currentLevelKnownBits;
	uint8_t buffer[9];
	uint8_t bufferUsed;
	uint8_t rxAlign;
	uint8_t txLastBits;
	uint8_t *responseBuffer;
	uint8_t responseLength;
	
	PCD_WriteRegister(CollReg, PCD_ReadRegister(CollReg) & 0x7F);
	buffer[0] = PICC_CMD_SEL_CL1;
	uidIndex = 0;
	currentLevelKnownBits = validBits - (8 * uidIndex);
	if (currentLevelKnownBits < 0)
		currentLevelKnownBits = 0;
	index = 2;
	uint8_t bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0);
	if (bytesToCopy) {
		if (bytesToCopy > 4)
			bytesToCopy = 4;
		for (count = 0; count < bytesToCopy; count++) {
			buffer[index++] = uid[uidIndex + count];
		}
	}
	while (!selectDone) {
		if (currentLevelKnownBits >= 32) {
			buffer[1] = 0x70;
			buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];
			if (!PCD_CalculateCRC(buffer, 7, &buffer[7]))
				return false;
			txLastBits		= 0;
			bufferUsed		= 9;
			responseBuffer	= &buffer[6];
			responseLength	= 3;
		}
		else {
			txLastBits		= currentLevelKnownBits % 8;
			count			= currentLevelKnownBits / 8;
			index			= 2 + count;
			buffer[1]		= (index << 4) + txLastBits;
			bufferUsed		= index + (txLastBits ? 1 : 0);
			responseBuffer	= &buffer[index];
			responseLength	= sizeof(buffer) - index;
		}
		
		rxAlign = txLastBits;
		PCD_WriteRegister(BitFramingReg, (txLastBits << 4) + txLastBits);
		uint8_t result = PCD_CommunicateWithPICC(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
		if (result == 2) {
			uint8_t valueOfCollReg = PCD_ReadRegister(CollReg);
			if (valueOfCollReg & 0x20)
				return false;
			uint8_t collisionPos = valueOfCollReg & 0x1F;
			if (collisionPos == 0)
				collisionPos = 32;
			if (collisionPos <= currentLevelKnownBits)
				return false;
			currentLevelKnownBits = collisionPos;
			count			= (currentLevelKnownBits - 1) % 8;
			index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0);
			buffer[index]	|= (1 << count);
		}
		else if (result != 1) {
			return false;
		}
		else {
			if (currentLevelKnownBits >= 32)
				selectDone = true;
			else
				currentLevelKnownBits = 32;
		}
	}
	
	index		= (buffer[2] == PICC_CMD_CT) ? 3 : 2;
	bytesToCopy = (buffer[2] == PICC_CMD_CT) ? 3 : 4;
	for (count = 0; count < bytesToCopy; count++) {
		uid[uidIndex + count] = buffer[index++];
	}
	if (!PCD_CalculateCRC(responseBuffer, 1, &buffer[2]) || responseLength != 3 || txLastBits != 0)
		return false;
	return true;
}


bool PICC_HaltA() {
	uint8_t buffer[4] = {PICC_CMD_HLTA, 0, 0, 0};
	if (PCD_CalculateCRC(buffer, 2, &buffer[2]))
		return !PCD_CommunicateWithPICC(buffer, sizeof(buffer));
	return false;
}
