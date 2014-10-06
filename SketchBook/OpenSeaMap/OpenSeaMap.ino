
/*
 OpenSeaMap.ino - Logger for the OpenSeaMap - Version 0.1.15
 Copyright (c) 2014 Wilfried Klaas.  All right reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 This program is a simple logger, which will take the input from 2 serial
 devices with the NMEA 0183 protokoll and write it down to a sd card.
 Additionally it will take the input from a onboard gyro device (MPU 6050)
 and write it into the same file. To every record it will add the timestamp
 of the arduino, so that the after processing process will determine the exact
 time of the event. (Like the GPS time).
 On startup and every hour the system will generate a new file.
 The programm can determine the shutdown and close all files.

 On the sd card can be a file named config.dat.
 First line is the baudrate of the NMEA A Port,
 Second line is the baudrate of the NMEA B Port

 If there ist no file, the default value will be used. Which is, both serial are active with
 standart NMEA0183 protokoll (4800, 8N1);
 0 = port is not active
 1 = 1200 Baud
 2 = 2400 Baud
 3 = 4800 Baud * Default
 4 = 9600 Baud (only NMEA A)
 5 = 19200 Baud (only NMEA A)
 6 = 38400 Baud (only NMEA A)
 
 for the first serial you can activate the SEATALK Protokoll, which has an other format.
 If you add an s before the baud value, seatalk protokoll will be activated.

 To Load firmware to OSM Lodder rename hex file to OSMFIRMW.HEX and put it on a FAT16 formatted SD card.
 */
// WKLA 20141006 V0.1.15
// - adding right bootloader constant
// - some testing with and without NMEA check
// WKLA 20140701 
// - rebuild for new 9N1 serial lib
// - overrun for the time at 18 hours
// - output config with a new NMEA message
// - in seatalk mode the baud rate is set to 4800 Baud
// - option for checking NMEA sectences
// - some comments for defines
// - getting the bootloader version
// - new version of AltSoftSerial
// - minimize send buffer for alt soft serial
// WKLA 20140512
// - vesselid in methode body to save ram
// WKLA 20140416 V0.1.12
// - every minute the logger will flush the file.
// - writing version number to eeprom for FAT32 Bootloader
// - new stop message with reason why logger creates a new file
// - calculating the right stop voltage
// WKLA 20140123 V0.1.10
// - selftest enhanced
// WKLA 20131123 V0.1.9
// - VesselID into EEPROM
// - New initialise section for better factory tests
// WKLA 20131120 V0.1.8
// - wait 30 seconds for logger start
// WKLA 20131120 V0.1.7
// - making gyro and vcc messages selectable with settings
// - writing the actual settings into a file called OSEAMLOG.CNF
// WKLA 20131110 V0.1.6
// - debug version
// WKLA 20131107 V0.1.5
// - for rev 3 boards now you can switch the 3V3 supply.
//   So you can reset the sd card without user interaction.
//   For rev2 and rev1 boards this option has no impact-
// WKLA 20131101 V0.1.4
// - Bug in reading config file fixed.
// WKLA 20131029 V0.1.3
// - testing free memory
// WKLA 20131028 V0.1.2
// - saving settings into EEPROM
// WKLA 20131027 V0.0.3
// - changing to AltSoftSerial
// WKLA 20131026
// - changes according to used memory
// - implementing 9N1 protocoll for seatalk
// - implementing seatalk protokoll
// - changing to SdFat library
// WKLA 20131010
// - implemeting crc for own data.

// define for the possibility of output of vcc messages
#define doOutputVcc

// define for the possibility of output of gyro messages
#define doOutputGyro

// define for the output of debug messages on serial 1
//#define debug

// define for output of memory messages to debug channel (so debug must be defined.)
//#define freemem

// checking a NMEA datagram before writing, if ok, channel LED will lite up.
#define checkNMEA

#ifdef freemem
#include <MemoryFree.h>
#endif

#ifdef doOutputGyro
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#endif

#include "messages.h"
#include "osm_debug.h"
#include "osm_makros.h"
#include "config.h"
#include <AltSoftSerial.h>

#include <SPI.h>
#include <SdFat.h>

#include <EEPROM.h>
#include "EEPROMStruct.h"
#include "osmfunctions.c"

#include <avr/pgmspace.h>
#include <util/crc16.h>

SdFat sd;
SdFile dataFile;

// actual activation state of the channel
boolean firstSerial = true;
boolean secondSerial = true;
boolean seatalkActive = false;
boolean outputGyro = true;
boolean outputVcc = false;

// Port for NMEA B
AltSoftSerial mySerial;

boolean error = false;

#ifdef doOutputGyro
MPU6050 accelgyro (MPU6050_ADDRESS_AD0_LOW);
#endif

byte indexA, indexB;

char filename[13];
unsigned long lastMillis;
int normVoltage;

void setup() {

  word firmVersion;
  EEPROM_readStruct(EEPROM_VERSION, firmVersion);
  if (firmVersion != VERSIONNUMBER) {
    firmVersion = VERSIONNUMBER;
    EEPROM_writeStruct(EEPROM_VERSION, firmVersion);
  }

  indexA = 0;
  indexB = 0;
  initDebug();

  // prints title with ending line break
  dbgOutLn(F("OpenSeaMap Datalogger"));
#ifndef debug
#ifdef freemem
  Serial.begin(4800);
  Serial.flush();
  delay(100);
#endif
#endif
  outputFreeMem('s');
  //--- init outputs and inputs ---
  // pins for LED's
  dbgOutLn(F("Init Ports"));
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_WRITE, OUTPUT);
  pinMode(LED_RX_A, OUTPUT);
  pinMode(LED_RX_B, OUTPUT);
  LEDAllOff();

  // Activating 3V3 supply
  pinMode(SUPPLY_3V3, OUTPUT);
  LEDOff(SUPPLY_3V3);

  // pins for switches
  pinMode(SW_STOP, INPUT_PULLUP);

  //--- init sd card ---
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SD_CHIPSELECT, OUTPUT);

  selftest();

  dbgOutLn(F("checking SD Card"));
  LEDAllOff();
  LEDOn(LED_POWER);

  // see if the card is present and can be initialized:
  // checking the sd card. There is the possibility, if the voltage drop below 3V but not to 0V
  // the sd card can struggle. Because of this, we switch off/on the 3V3 Voltage, to make a card reset.
  while (!sd.begin(SD_CHIPSELECT)) {
    dbgOutLn(F("Card failed, or not present, restarting"));
    // Card reset
    LEDOn(LED_WRITE);
    LEDOff(SUPPLY_3V3);
    delay(800);
    LEDOff(LED_WRITE);
    LEDOn(SUPPLY_3V3);
    delay(500);
  }
  dbgOutLn(F("SD Card ready"));

  LEDOn(SUPPLY_3V3);
  LEDAllOff();

  // just waiting 30 seconds for cap loading...
  LEDAllOff();
  waitCapLoad();
  LEDOn(LED_POWER);

  // loading default parameters, if config file is present. Then init all serial communications
  LEDOn(LED_RX_A);
  dbgOutLn(F("Init NMEA"));
  getParameters();

  // just init the gyro again (because of the possible drop of the 3V3 voltage.
  LEDOn(LED_RX_B);
  initGyro();
  delay(500);
  LEDAllOff();
}

/**
 * Just waiting some seconds for loading the Goldcap, befor we start operation.
 **/
inline void waitCapLoad() {
  dbgOutLn(F("CAP-load"));
  // save time for cap loading...
  lastMillis = millis() + GOLDCAP_LOADING_TIME;
  byte count = 0;
  unsigned long sumVoltage = 0;
  while (millis() < lastMillis) {
    sumVoltage += readVcc();
    count++;
    LEDOn(LED_POWER);
    delay(500);
    LEDOff(LED_POWER);
    delay(500);
  }
  normVoltage = (sumVoltage / count) - VCC_GOLDCAP;
}

/**
 * Selftest of the unit. First we "test" the supply. LED Green.
 * Than we switch on the 3V3 supply. LED RX A (first LED in RJ45)
 * Than we test the gyro. LED RX B (second LED in RJ45)
 * If everything works fine, after all test all LED's, without the WRITE LED, will be lit.
 **/
inline void selftest() {
  // Power LED on
  LEDOn(LED_POWER);

  // switching the 3V3 supply on.
  delay(500);
  LEDOn(LED_RX_A);
  LEDOn(SUPPLY_3V3);
  delay(1000);

  // init the gyro, if needed
  LEDOn(LED_RX_B);
  initGyro();
  delay(1000);
}

/**
 * Getting the actual config parameters and configure.
 * first try to get parameters from the EEPROM
 * than try to load the config.dat file from sd card.
 * if file exists, load parameters from file and save changed parameters to EEPROM
 **/
void getParameters() {
  dbgOutLn(F("readconf"));
  strcpy_P(filename, CONFIG_FILENAME);
  byte baudA = EEPROM.read(EEPROM_BAUD_A);
  byte baudB = EEPROM.read(EEPROM_BAUD_B);
  byte seatalk = EEPROM.read(EEPROM_SEATALK);
  byte outputs = EEPROM.read(EEPROM_OUTPUT);
  unsigned long vesselID = 0;
  EEPROM_readStruct(EEPROM_VESSELID, vesselID);

  byte bootloaderVersion = EEPROM.read(EEPROM_BOOTLOADER_VERSION);
  if (bootloaderVersion > 10) {
    bootloaderVersion = 1;
    EEPROM.write(EEPROM_BOOTLOADER_VERSION, bootloaderVersion);
  }

  word crc = CalculateChecksum(0x7E00, 512);
  if ((crc == BOOTLOADER_2_CONST) && (bootloaderVersion != 2)) {
    bootloaderVersion = 2;
    EEPROM.write(EEPROM_BOOTLOADER_VERSION, bootloaderVersion);
  }

  seatalkActive = false;

  dbgOut(F("EEPROM read:"));
  dbgOut2(baudA, HEX);
  dbgOut(',');
  dbgOut2(baudB, HEX);
  dbgOut(',');
  dbgOut2(seatalk, HEX);
  dbgOut(',');
  dbgOutLn2(outputs, HEX);
  dbgOut(',');
  dbgOutLn2(bootloaderVersion, HEX);
  dbgOut(',');
  dbgOutLn2(crc, HEX);

  if (baudA > 0x06) {
    baudA = 3;
  }
  if (baudB > 0x04) {
    baudB = 3;
  }

  if (seatalk < 0x06) {
    seatalkActive = seatalk > 0;
  }

  if (sd.exists(filename)) {
    dbgOutLn(F("file exists"));
    if (dataFile.open(filename, O_RDONLY)) {
      dbgOutLn(F("file open"));
      byte paramCount = 1;
      boolean lastCR = false;
      firstSerial = false;
      secondSerial = false;
      while (dataFile.available()) {
        byte readValue = dataFile.read();
        dbgOut(F("value:"));
        dbgOut(readValue);
        dbgOut(F(" pcnt:"));
        dbgOutLn(paramCount);
        if ((readValue == 0x0D) || (readValue == 0x0A)) {
          if (!lastCR) {
            paramCount++;
            lastCR = true;
          }
        }
        else {
          lastCR = false;
          if (paramCount == 1) {
            if (readValue == 's') {
              dbgOutLn(F("ST active"));
              seatalkActive = true;
              readValue = dataFile.read();
            } else {
              dbgOutLn(F("ST inactive"));
              seatalkActive = false;
            }
            byte baud = readValue - '0';
            dbgOut(F("A baud readed:"));
            dbgOutLn(BAUDRATES[baud]);
            if (baud != baudA) {
              dbgOutLn(F("EEPROM write A:"));
              EEPROM.write(EEPROM_BAUD_A, baud);
            }
            if ((seatalk > 0x06) || ((seatalk > 0) != seatalkActive)) {
              dbgOutLn(F("EEPROM write SEATALK:"));
              if (seatalkActive) {
                EEPROM.write(EEPROM_SEATALK, 1);
              }
              else {
                EEPROM.write(EEPROM_SEATALK, 0);
              }
            }
            baudA = baud;
          }
          if (paramCount == 2) {
            byte baud = readValue - '0';
            dbgOut(F("B baud readed:"));
            dbgOutLn(BAUDRATES[baud]);
            if (baud != baudB) {
              dbgOutLn(F("EEPROM write B:"));
              EEPROM.write(EEPROM_BAUD_B, baud);
            }
            baudB = baud;
          }
          if (paramCount == 3) {
            byte foutputs = readValue - '0';
            dbgOut(F("Outputs readed:"));
            dbgOutLn(foutputs);
            if (foutputs != outputs) {
              dbgOutLn(F("EEPROM write Outputs:"));
              EEPROM.write(EEPROM_OUTPUT, foutputs);
            }
            outputs = foutputs;
          }
          if (paramCount == 4) {
            // read vesselID
            vesselID = 0;
            byte pos = 0;
            filename[pos++] = readValue;
            while (dataFile.available()) {
              readValue = dataFile.read();
              if ((readValue == 0x0D) || (readValue == 0x0A)) {
                break;
              }
              filename[pos++] = readValue;
            }
            filename[pos] = 0;
            vesselID = strtoul(filename, NULL, 16);
            dbgOut(F("Vesselid:"));
            dbgOutLn2(vesselID, HEX);
            EEPROM_writeStruct(EEPROM_VESSELID, vesselID);
          }
        }
      }
      dataFile.close();
    }
  }

  if (outputs < 0x80) {
    outputVcc = (outputs & 0x01) > 0;
    outputGyro = (outputs & 0x02) > 0;
  }

  outputParameter(baudA, baudB, outputs, vesselID, bootloaderVersion, crc);

  initSerials(baudA, baudB);
}

/**
 * writing parameter to oseamlog.cnf file.
 * So on every sd card you will have the actual parameters of the logger.
 **/
inline void outputParameter(byte baudA, byte baudB, byte outputs, unsigned long vesselID, byte bootloaderVersion, word crc) {
  strcpy_P(filename, CNF_FILENAME);
  if (sd.exists(filename)) {
    sd.remove(filename);
  }

  dataFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
  if (seatalkActive) {
    dataFile.print('s');
  }
  dataFile.println(baudA);
  dataFile.println(baudB);
  dataFile.println(outputs);
  strcpy_P(filename, VERSION);
  dataFile.println(filename);
  dataFile.println(vesselID, HEX);
  dataFile.println(normVoltage);
  dataFile.println(bootloaderVersion);
  dataFile.println(crc, HEX);

  dataFile.close();
}

/**
 * initialsie the serial communication hardware.
 * For seatalk we need the 9N1 Protokoll. Otherwise we initialise with 8N1.
 * For seatalk we only have a baudrate of 4800.
 * For NMEA we  can use other.
 **/
inline void initSerials(byte baudA, byte baudB) {
  dbgOutLn(F("Init Searials"));
  word baud = 0;
  if (baudA < 0x06) {

    baud = BAUDRATES[baudA];

#ifdef debug
    if (seatalkActive) {
      dbgOut(F("E SK:"));
    }
    else {
      dbgOut(F("E NA:"));
    }
    Serial.println(baud, DEC);
#endif
    Serial.end();

    // init serial channel a
    if (baudA > 0) {
      firstSerial = true;
      // for seatalk we need another initialisation
      if (seatalkActive) {
        Serial.begin(4800, SERIAL_9N1);
      } else {
        Serial.begin(baud, SERIAL_8N1);
      }
    }
  }


  // init serial for channel b
  mySerial.end();
  if (baudB < 0x04) {
    baud = BAUDRATES[baudB];

#ifdef debug
    dbgOut(F("E NB:"));
    Serial.println(baud, DEC);
#endif

    if (baud > 0) {
      secondSerial = true;
      mySerial.begin(baud);
    }
  }
}

/**
 * Initialise the gyro and acc. For gyro we take 250°/sec, for the acc we use 2g
 **/
inline void initGyro() {
  //--- init MPU6050 ---
#ifdef doOutputGyro
  dbgOutLn(F("Init I2C"));

  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();
  // initialize device
  accelgyro.initialize();

  if (accelgyro.testConnection()) {
    dbgOutLn(F("MPU6050 successful"));
    accelgyro.setFullScaleAccelRange(0);
    accelgyro.setFullScaleGyroRange(0);
  }
  else {
    LEDOff(LED_RX_B);
    dbgOutLn(F("MPU6050 failed"));
  }
#endif
}

/**
 * calcualting the 16 bit checksum of a part of the flash memory.
 **/
static word CalculateChecksum (word addr, word size) {
  word crc = ~0;

/*  prog_uint8_t* p = (prog_uint8_t*) addr;
  for (word i = 0; i < size; ++i) {
    crc = _crc16_update(crc, pgm_read_byte(p++));
  }
  */
  word endCrc = addr + size;
  for (word i = addr; i < endCrc; ++i) {
    crc = _crc16_update(crc, pgm_read_byte((void *)i));
  }
  
  return crc;
}

/*********************************/
/*           Main part           */
/*********************************/

byte fileCount = 0;
int ax, ay, az;
word vcc;
unsigned long lastW;
unsigned long vccTime;
unsigned long startA, startB;

// the 3 buffers for transfering data.
byte bufferA[MAX_NMEA_BUFFER];
byte bufferB[MAX_NMEA_BUFFER];
char linedata[MAX_NMEA_BUFFER];
char timedata[15];

byte lastFlush = 0;

/**
 * main loop.
 **/
void loop() {
  // check LED State
  long now = millis();
  checkLEDState(now);

  // read power supply
  vcc = readVcc();
  // if the voltage drops below 4V7 we are on Cappower, so we must close all files and wait.
  if ((vcc < normVoltage) || (digitalRead(SW_STOP) == 0)) {
    // Alle LED's aus, Strom sparen
    LEDAllOff();

    if (dataFile.isOpen()) {

      writeVCC();
      if (vcc < normVoltage) {
        strcpy_P(linedata, REASON_VCC_MESSAGE);
      } else {
        strcpy_P(linedata, REASON_SWITCH_MESSAGE);
      }
      writeData(millis(), CHANNEL_I_IDENTIFIER, linedata);  // write data to card
      stopLogger();
      dbgOutLn(F("Shutdown detected, datafile closed"));
    }
    // switch on all LED's, to unload the gold cap.
    LEDAllOn();
    delay(10000);
  }
  else {
    if (!dataFile.isOpen()) {
      newFile();
    }

    testSerialA();
    testSerialB();

    // check the gyro only once in a second
    //now = millis();
    if ((now - 1000) > lastMillis) {
      outputFreeMem('L');
      lastMillis = now;

      writeGyroData();
      writeVCC();

      byte nowFlush = (now / 60000L) % 256L;
      word nowCount = (now / 1000L) / 3600L;

      // testing the needing of a new file
      if (nowCount > fileCount) {
        strcpy_P(linedata, REASON_TIME_MESSAGE);
        writeData(millis(), CHANNEL_I_IDENTIFIER, linedata);  // write data to card
        newFile();
        fileCount++;
      } else if (nowFlush != lastFlush) {
        flushFile();
        lastFlush = nowFlush;
      }
    }
  }
}

/**
 * checking the state of all LED's.
 * On error blink with the power LED (2Hz)
 * Power LED must be lit
 * sd write LED musst be unlit, if it's lit more than 100 ms
 * RX A LED musst be unlit, if it's lit more than 500 ms
 * RX B LED musst be unlit, if it's lit more than 500 ms
 **/
inline void checkLEDState(long now) {
  // if an error occur, just blink with the power LED.
  if (error) {
    dbgOutLn(F("ERROR"));
    if ((now % 500L) > 250) {
      LEDOff(LED_POWER);
    }
    else {
      LEDOn(LED_POWER);
    }
  }
  else {
    LEDOn(LED_POWER);
  }

  if (now > (lastW + 100)) {
    LEDOff(LED_WRITE);
  }

  // reset LED for receiving channel A
  if (now > (startA + 500)) {
    LEDOff(LED_RX_A);
  }

  // reset LED for receiving channel B
  if (now > (startB + 500)) {
    LEDOff(LED_RX_B);
  }
}

/**
 * reading the actual voltage of the board.
 **/
word readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
#ifdef doOutputVcc
  vccTime = millis();
#endif
  return result;
}

/**
 * writing vcc data to the sd card.
 **/
inline void writeVCC() {
#ifdef doOutputVcc
  if (outputVcc) {
    sprintf_P(linedata, VCC_MESSAGE, vcc, normVoltage);
    writeData(vccTime, CHANNEL_I_IDENTIFIER, linedata);
  }
#endif
}

/**
 * writing gyro data to the sd card.
 **/
inline void writeGyroData() {
#ifdef doOutputGyro
  if (outputGyro) {
    unsigned long startTime = millis();
    accelgyro.getRotation(&ax, &ay, &az);
    sprintf_P(linedata, GYRO_MESSAGE, ax, ay, az);
    writeData(startTime, CHANNEL_I_IDENTIFIER,  linedata);

    accelgyro.getAcceleration(&ax, &ay, &az);
    sprintf_P(linedata, ACC_MESSAGE, ax, ay, az);
    writeData(startTime, CHANNEL_I_IDENTIFIER, linedata);
  }
#endif
}

/**
 * writing config data as NMEA message to the data file
 **/
void outputConfig() {
  unsigned long startTime = millis();
  byte baudA = EEPROM.read(EEPROM_BAUD_A);
  byte baudB = EEPROM.read(EEPROM_BAUD_B);
  byte seatalk = EEPROM.read(EEPROM_SEATALK);
  byte outputs = EEPROM.read(EEPROM_OUTPUT);
  unsigned long vesselID = 0;
  EEPROM_readStruct(EEPROM_VESSELID, vesselID);
  byte bootloaderVersion = EEPROM.read(EEPROM_BOOTLOADER_VERSION);

  sprintf_P(linedata, CONFIG_MESSAGE, baudA, baudB, seatalk, outputs, vesselID, bootloaderVersion);
  writeData(startTime, CHANNEL_I_IDENTIFIER,  linedata);
}

/**
 * flushnig the data file. (Will be calles once a minute)
 **/
void flushFile() {
  dataFile.close();
  dataFile.open(filename, O_RDWR | O_APPEND | O_AT_END);
}

word lastStartNumber = 0;

/**
 * creating  a new file with a new filename on the sd card.
 **/
void newFile() {
  stopLogger();
  LEDAllOff();
  while (!sd.begin(SD_CHIPSELECT, SPI_HALF_SPEED)) {
    dbgOutLn(F("Card failed, or not present, restarting"));
    //    PORTD ^= _BV(LED_WRITE);
    LEDAllBlink();
    //    LEDOff(SUPPLY_3V3);
    outputFreeMem('F');
    delay(500);
    LEDAllBlink();
    //    LEDOn(SUPPLY_3V3);
    delay(500);
  }
  int t = 0;
  int h = 0;
  int z = 0;
  int e = 0;
  strcpy_P(filename, DATA_FILENAME);
  LEDAllOff();

  for (word i = lastStartNumber + 1; i < 10000; i++) { // create new filename w/ 3 digits 000-999
    LEDBlink(LED_POWER);
    t = i / 1000;
    filename[4] = t + '0';          // calculates tausends position
    h = (i - (t * 1000)) / 100;
    filename[5] = h + '0';          // calculates hundred position
    z = (i - (t * 1000) - (h * 100)) / 10 ;
    filename[6] = z + '0';          // subtracts hundreds & tens for single digit
    e = i - (t * 1000) - (h * 100) - (z * 10);
    filename[7] = e + '0';          // calculates hundreds position
    if (! sd.exists(filename)) {    // only open a new file if it doesn't exist
      // checking the freespace
      uint32_t freeKB = sd.vol()->freeClusterCount();
      // calculating the count of minimun required culsters
      uint32_t required = 100 * 1024 * 2 / sd.vol()->blocksPerCluster(); // min 100MB sollten noch frei sein
      if (freeKB >= required) {
        dataFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
        lastStartNumber = i;
      }
      else {
        error = true;
      }
      break;                        // leave the loop after finding new filename
    }
  }

  strcpy_P(linedata, START_MESSAGE);
  dbgOutLn(F("Start"));
  writeData(millis(), CHANNEL_I_IDENTIFIER, linedata);          // write data to card
  outputConfig();
}

/**
 * stopping the logger, writing stop message, closing data file.
 **/
void stopLogger() {
  dbgOutLn(F("close datafile."));
  strcpy_P(linedata, STOP_MESSAGE);
  writeData(millis(), CHANNEL_I_IDENTIFIER, linedata);  // write data to card
  if (dataFile.isOpen()) {
    dataFile.close();
  }
}

bool endingA, endingB;

/**
 * testing the first serial connection.
 **/
void testSerialA() {
  if (firstSerial) {
    outputFreeMem('1');
    endingA = false;
    while ((Serial.available()  > 0) && !endingA) {
      int incomingByte = Serial.read();
      if (incomingByte >= 0) {
        if (indexA == 0) {
          startA = millis();
        }
#ifndef checkNMEA
        LEDOn(LED_RX_A);
#endif
        if (seatalkActive) {
          SeaTalkInputA(incomingByte);
        }
        else {
          NMEAInputA(incomingByte);
        }
      }
    }
  }
}


/**
 * processing seatalk data.
 **/
byte dataLength;
inline void SeaTalkInputA(int incomingByte) {
  if ((incomingByte & 0x0100) > 0) {
    writeDatagram();
    endingA = true;
    indexA = 0;
    startA = millis();
  }
  if (indexA == 1) {
    dataLength = 3 + (incomingByte & 0x0F);
  }
  if (indexA < MAX_NMEA_BUFFER) {
    bufferA[indexA] = (byte) incomingByte;
    indexA++;
  }
}

/**
 * writing seatalk datagram.
 **/
inline void writeDatagram() {
  if (indexA == dataLength) {
    dbgOutLn(SEATALK_NMEA_MESSAGE);
    strcpy_P(linedata, SEATALK_NMEA_MESSAGE);

    for (byte i = 0; i < indexA; i++) {
      byte value = bufferA[i];
      byte c = (value & 0xF0) >> 4;
      strcat(linedata, convertNibble2Hex(c));
      c = value & 0x0F;
      strcat(linedata, convertNibble2Hex(c));
    }

    writeData(startA, CHANNEL_A_IDENTIFIER, linedata);
    indexA = 0;
  }
}

/**
 * processing NMEA data.
 **/
inline void NMEAInputA(int incomingByte) {
  byte in = lowByte(incomingByte);
  if (in == 0x0A) {
    endingA = true;
  }
  else {
    if (in != 0x0D) {
      bufferA[indexA] = (byte) in;
      indexA++;
    }
  }
  if (endingA || (indexA >= MAX_NMEA_BUFFER)) {
    if (indexA > 0) {
#ifdef debug
      if (indexA >= MAX_NMEA_BUFFER) {
        Serial.print('B');
      }
      Serial.print(F("A:"));
      if (indexA > 6) {
        Serial.write(bufferA, 6);
      }
      else {
        Serial.write(bufferA, indexA);
      }
      Serial.println();
#endif
      if (dataFile.isOpen()) {
#ifdef checkNMEA
        if (checkNMEAData(bufferA)) {
          LEDOn(LED_RX_A);
        }
#endif
        writeLEDOn();
        writeTimeStamp(startA);
        writeChannelMarker(CHANNEL_A_IDENTIFIER);
        dataFile.write(bufferA, indexA);
        dataFile.println();
      }
      indexA = 0;
    }
  }
}

/**
 * testing the second serial connection.
 **/
void testSerialB() {
  if (secondSerial) {
    outputFreeMem('2');
    endingB = false;
    while ((mySerial.available()  > 0) && !endingB) {
      int incomingByte = mySerial.read();
      if (incomingByte >= 0) {
        if (indexB == 0) {
          startB = millis();
        }
#ifndef checkNMEA
        LEDOn(LED_RX_B);
#endif
        NMEAInputB(incomingByte);
      }
    }
  }
}

/**
 * processing NMEA data.
 **/
inline void NMEAInputB(int incomingByte) {
  byte in = lowByte(incomingByte);
  if (in == 0x0A) {
    endingB = true;
  }
  else {
    if (in != 0x0D) {
      bufferB[indexB] = (byte) in;
      indexB++;
    }
  }
  if (endingB || (indexB >= MAX_NMEA_BUFFER)) {
    if (indexB > 0) {
#ifdef debug
      if (indexB >= MAX_NMEA_BUFFER) {
        Serial.print('B');
      }
      Serial.print(F("B:"));
      if (indexB > 6) {
        Serial.write(bufferB, 6);
      }
      else {
        Serial.write(bufferB, indexB);
      }
      Serial.println();
#endif
      if (dataFile.isOpen()) {
#ifdef checkNMEA
        if (checkNMEAData(bufferB)) {
          LEDOn(LED_RX_B);
        }
#endif
        writeLEDOn();
        writeTimeStamp(startB);
        writeChannelMarker(CHANNEL_B_IDENTIFIER);
        dataFile.write(bufferB, indexB);
        dataFile.println();
        //        writeLEDOff();
      }
      indexB = 0;
    }
  }
}

/**
 * checking if the NMEA Data is correct
 **/
bool checkNMEAData(byte* myBuffer) {
  char* data = (char*) myBuffer;
  if (strlen(data) == 0) {
    return false;
  }
  if (data[0] != '$') {
    return false;
  }
  bool inCrc = false;
  byte crc = 0;
  byte fileCrc = 0;
  byte index = 0;
  for (byte i = 1; i < strlen(data); i++) {
    char value = data[i];
    if ((value < 0x20) || (value > 0x80)) {
      return false;
    }
    if (inCrc) {
      if (index < 2) {
        if (value >= '0' && value <= '9') {
          fileCrc = value - '0';
        }
        if (value >= 'A' && value <= 'F') {
          fileCrc = value - 'A';
        }
        if (index == 0) {
          fileCrc = fileCrc << 4;
        }
        index++;
      }
    } else {
      if (value != '*') {
        crc ^= value;
      } else {
        inCrc = true;
      }
    }
  }

  if (fileCrc != crc) {
    return false;
  }
  return true;
}

/**
 * writing a new logger entry.
 **/
void writeData(unsigned long startTime, char marker, char* data) {
  if (dataFile.isOpen()) {
    writeTimeStamp(startTime);
    writeChannelMarker(marker);
    writeNMEAData(data);
  }
}

/**
 * writing the timestamp.
 **/
void writeTimeStamp(unsigned long time) {
  word mil = time % 1000L;
  long div = time / 1000L;

  byte sec = div % 60L;
  byte minute = (div / 60L) % 60L;
  byte hour = (div / 3600L) % 24L;

  sprintf_P(timedata, TIMESTAMP, hour, minute, sec, mil);
  dataFile.print(timedata);
}

/**
 * writing the channel marker.
 **/
void writeChannelMarker(char marker) {
  dataFile.print(marker);
  dataFile.print(';');
}

/**
 * writing a new NMEA Datagram.
 **/
void writeNMEAData(char* data) {
  writeLEDOn();
  byte crc = 0;
  for (byte i = 0; i < strlen(data); i++) {
    crc ^= data[i];
  }
  dataFile.write('$');
  dataFile.print(data);
  dataFile.write('*');
  if (crc < 16) {
    dataFile.write('0');
  }
  dataFile.println(crc, HEX);
#ifdef debug
  dbgOut('$');
  dbgOut(data);
  dbgOut('*');
  if (crc < 16) {
    dbgOut('0');
  }
  dbgOutLn2(crc, HEX);
#endif
}
