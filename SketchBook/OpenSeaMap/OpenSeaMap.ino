//#define doOutputVcc
#define doOutputGyro
//#define debug
//#define freemem
//#define noWrite
//#define devmode

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
/*
 OpenSeaMap.ino - Logger for the OpenSeaMap - Version 0.1.9
 Copyright (c) 2013 Wilfried Klaas.  All right reserved.
 
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
 */
/*
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
 0 port is not active
 1 = 1200 Baud
 2 = 2400 Baud 
 3 = 4800 Baud * Default
 4 = 9600 Baud (only NMEA A)
 5 = 19200 Baud (only NMEA A)
 
 for the first serial you can activate the SEATALK Protokoll, which has an other format. 
 If you add an s before the baud value, seatalk protokoll will be activated.
 
 To Load firmware to OSM Lodder rename hex file to OSMFIRMW.HEX and put it on a FAT16 formatted SD card. 
 */
// WKLA 20131123 V0.1.9
// - VesselID ins EEPROM
// - New initialise section for better factory tests
// WKLA 20131120 V0.1.8
// - 30 Sekunden warten, bis Logger startet
// WKLA 20131120 V0.1.7
// - Gyro und VCC per configdatei wählbar machen
// - Datei mit den aktuellen Einstellungen auf SD schreiben
// WKLA 20131110 V0.1.6
// - Gyro und VCC per configdatei wählbar machen
// WKLA 20131107 V0.1.5
// - Für rev 3 Boards, kann nun die 3V3 Spannungsversorgung geschaltet werden. 
//   Dieses wird immer beim fehlerhaften Start der SD Karte gemacht. Für rev 2 
//   und rev 1 Board hat das Verhalten keinen Einfluss.
// WKLA 20131101 V0.1.4
// - Bug in Config Datei lesen behoben.
// WKLA 20131029 V0.1.3
// - Testen auf freien Platz
// WKLA 20131028 V0.1.2
// - EEPROM für Konfiguration
// WKLA 20131027 V0.0.3
// - Umstellung auf AltSoftSerial
// WKLA 20131026 
// - diverse Änderungen wegen Speichervervrauch
// - 9N1 Protokoll für Seatalk
// - Seatalk implementierung
// - Umstellung auf SdFat Bibliothek
// WKLA 20131010
// - Prüfsumme für Daten eingebaut.

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

byte index_a, index_b;

char filename[13];
unsigned long lastMillis;
unsigned long vesselID;

void setup() {
  index_a = 0;
  index_b = 0;
  initDebug();

  // prints title with ending line break
  dbgOutLn(F("OpenSeaMap Datalogger"));
#ifdef debug
#ifdef devmode
  dbgOutLn(F("Devmode"));
#endif
#endif
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

  // Power LED on
  LEDOn(LED_POWER);

  // see if the card is present and can be initialized:
  delay(1000);
  LEDOn(SUPPLY_3V3);

  initGyro();
  delay(1000);

  dbgOutLn(F("checking SD Card"));
  while (!sd.begin(SD_CHIPSELECT)) {
    dbgOutLn(F("Card failed, or not present, restarting"));
    LEDAllBlink();
    LEDOff(SUPPLY_3V3);
    outputFreeMem('F');
    delay(500);
    LEDAllBlink();
    LEDOn(SUPPLY_3V3);
    delay(500);    
  }
  dbgOutLn(F("SD Card ready"));

  LEDAllOff();
  LEDOn(LED_POWER);
   
#ifndef devmode
  LEDAllOff();
  dbgOutLn(F("CAP-load"));
  lastMillis = millis() + 30000;
  while (millis() < lastMillis) {
    LEDAllBlink();
    delay(500);
    LEDAllBlink();
    delay(500);
  }
#endif

  LEDOn(LED_RX_A);
  dbgOutLn(F("Init NMEA"));
  getParameters();

  initGyro();
}

/**
 * Getting the actual config parameters and configure.
 * first try to get parameters from the EEPROM
 * than try to load the config.dat file from sd card.
 * if exists, load parameters from file and save changed parameters to EEPROM
 * return <true> if parameters could be loaded and configured, otherwiese <false>
 **/
void getParameters() {
  dbgOutLn(F("readconf"));
  strcpy_P(filename, CONFIG_FILENAME);
  byte baud_a = EEPROM.read(EEPROM_BAUD_A);
  byte baud_b = EEPROM.read(EEPROM_BAUD_B);
  byte seatalk = EEPROM.read(EEPROM_SEATALK);
  byte outputs = EEPROM.read(EEPROM_OUTPUT);
  EEPROM_readStruct(EEPROM_VESSELID, vesselID);
  
  seatalkActive = false;

  dbgOut(F("EEPROM read:"));
  dbgOut2(baud_a,HEX);
  dbgOut(',');
  dbgOut2(baud_b,HEX);
  dbgOut(',');
  dbgOut2(seatalk,HEX);
  dbgOut(',');
  dbgOutLn2(outputs,HEX);

  if (baud_a > 0x06) {
    baud_a = 3;
  }
  if (baud_b > 0x04) {
    baud_b = 3;
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
            if (baud != baud_a) {
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
            baud_a = baud;
          }
          if (paramCount == 2) {
            byte baud = readValue - '0';
            dbgOut(F("B baud readed:"));
            dbgOutLn(BAUDRATES[baud]);
            if (baud != baud_b) {
              dbgOutLn(F("EEPROM write B:"));
              EEPROM.write(EEPROM_BAUD_B, baud);
            }       
            baud_b = baud;
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
  
  outputParameter(baud_a, baud_b, outputs);
  
  initSerials(baud_a, baud_b);
}

inline void outputParameter(byte baud_a, byte baud_b, byte outputs) {
  strcpy_P(filename, CNF_FILENAME);
  if (sd.exists(filename)) {
    sd.remove(filename);
  }
  
  dataFile.open(filename, O_RDWR | O_CREAT | O_AT_END);
  if (seatalkActive) {
    dataFile.print('s');
  }
  dataFile.println(baud_a);
  dataFile.println(baud_b);
  dataFile.println(outputs);
  strcpy_P(filename, VERSION);
  dataFile.println(filename);
  dataFile.println(vesselID, HEX);
  dataFile.close();
}

inline void initSerials(byte baud_a, byte baud_b) {
  dbgOutLn(F("Init Searials"));
  word baud = 0;
  if (baud_a < 0x06) {
  
    baud = BAUDRATES[baud_a];

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

    if (baud_a > 0) {
      firstSerial = true;
      // for seatalk we need another initialisation 
      if (seatalkActive) {
        Serial.begin(baud, SERIAL_9N1);
      }
      else {
        Serial.begin(baud, SERIAL_8N1);
      }              
    }
  }


  mySerial.end();
  if (baud_b < 0x04) {
    baud = BAUDRATES[baud_b];

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

inline void initGyro() {
  //--- init MPU6050 ---
  // join I2C bus (I2Cdev library doesn't do this automatically)
  dbgOutLn(F("Init I2C"));
  LEDOn(LED_RX_B);

#ifdef doOutputGyro
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

/*********************************/
/*           Main part           */
/*********************************/

byte fileCount = 0;

#ifdef doOutputGyro
int16_t ax, ay, az;
#endif

word vcc;
unsigned long lastW;

#ifdef doOutputVcc
unsigned long vccTime;
#endif

unsigned long now, start_a, start_b;

byte buffer_a[MAX_NMEA_BUFFER];
byte buffer_b[MAX_NMEA_BUFFER];
char linedata[MAX_NMEA_BUFFER];
char timedata[15];

/**
 * main loop.
 **/
void loop() {
  // im Fehlerfall blinken
  if (error) {
    dbgOutLn(F("ERROR"));
    if ((millis() % 500) > 250) {
      LEDOff(LED_POWER);
    } 
    else {
      LEDOn(LED_POWER);
    }
  } 
  else {
    LEDOn(LED_POWER);
  }

  // check LED State
  long now = millis();
  if (now > (lastW + 100)) {
    LEDOff(LED_WRITE);
  }

  // reset LED for receiving channel A 
  if (now > (start_a + 500)) {
    LEDOff(LED_RX_A);
  }

  // reset LED for receiving channel B 
  if (now > (start_b + 500)) {
    LEDOff(LED_RX_B);
  }

  // Versorgungsspannung messen
  vcc = readVcc();
  // Bei Unterspannung, alles herrunter fahren
  if ((vcc < VCC_GOLDCAP) || (digitalRead(SW_STOP)==0)) {
    // Alle LED's aus, Strom sparen
    LEDAllOff();

    if (dataFile.isOpen()) {
#ifdef doOutputVcc
      writeVCC();
#endif
      stopLogger();
      dbgOutLn(F("Shutdown detected, datafile closed"));
    }
    // Jetzt alle LED's einschalten, damit ordentlich strom verbraucht wird.
    LEDAllOn();
    delay(10000);
  } 
  else {
    if (!dataFile.isOpen()) {
      newFile();
    }

    testFirstSerial();
    testSecondSerial();

    // der Gyro wird nur jede Sekunde einmal abgefragt
    //now = millis();
    if ((now - 1000) > lastMillis) {
      outputFreeMem('L');
      lastMillis = now;
#ifdef doOutputGyro
      writeGyroData();
#endif      
#ifdef doOutputVcc
      writeVCC();
#endif      
      // testing the needing of a new file
      if ((now / 3600000) > fileCount) {
        newFile(); 
        fileCount++;
      }       
    }
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
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
#ifdef doOutputVcc
  vccTime = millis();
#endif
  return result;
}

/**
 * writing vcc data to the sd card.
 **/
#ifdef doOutputVcc
inline void writeVCC() {
  if (outputVcc) {
    sprintf_P(linedata, VCC_MESSAGE, vcc); 
    writeData(vccTime, 'I', linedata);
  }
}
#endif

/**
 * writing gyro data to the sd card.
 **/
#ifdef doOutputGyro
inline void writeGyroData() {
  if (outputGyro) {
    unsigned long startTime = millis();
    accelgyro.getRotation(&ax, &ay, &az);
    sprintf_P(linedata, GYRO_MESSAGE, ax, ay, az); 
    writeData(startTime, 'I',  linedata);

    accelgyro.getAcceleration(&ax, &ay, &az);
    sprintf_P(linedata, ACC_MESSAGE, ax, ay, az); 
    writeData(startTime, 'I', linedata);
  }
}
#endif

word lastStartNumber = 0;

/**
 * creating  a new file with a new filename on the sd card.
 **/
void newFile() {
  stopLogger();
  LEDAllOff();
#ifndef noWrite
  while (!sd.begin(SD_CHIPSELECT, SPI_HALF_SPEED)) {
    dbgOutLn(F("Card failed, or not present, restarting"));
    //    PORTD ^= _BV(LED_WRITE);
    LEDAllBlink();
    LEDOff(SUPPLY_3V3);
    outputFreeMem('F');
    delay(500);
    LEDAllBlink();
    LEDOn(SUPPLY_3V3);
    delay(500);
  }
  int t = 0; 
  int h = 0;
  int z = 0;
  int e = 0;
  strcpy_P(filename, DATA_FILENAME);

  for (word i = lastStartNumber+1; i < 10000; i++) { // create new filename w/ 3 digits 000-999
    t = i/1000;
    filename[4] = t + '0';          // calculates tausends position
    h = (i - (t*1000)) / 100;
    filename[5] = h + '0';          // calculates hundred position
    z = (i - (t*1000) - (h*100)) / 10 ;
    filename[6] = z + '0';          // subtracts hundreds & tens for single digit
    e = i - (t*1000) - (h*100) - (z*10);
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
#endif
  strcpy_P(linedata, START_MESSAGE);
  dbgOutLn(F("Start"));
  writeData(millis(), 'I', linedata);          // write data to card
}

/**
 * stopping the logger, writing stop message, closing data file.
 **/
void stopLogger() {
  dbgOutLn(F("close datafile."));
  strcpy_P(linedata, STOP_MESSAGE);
  writeData(millis(), 'I', linedata);  // write data to card
  if (dataFile.isOpen()) {
    dataFile.close();
  }
}

bool ending1, ending2;

/**
 * testing the first serial connection.
 **/
void testFirstSerial() {
  if (firstSerial) {
    outputFreeMem('1');
    ending1 = false;
    while ((Serial.available()  > 0) && !ending1) {
      int incomingByte = Serial.read();
      if (incomingByte >= 0) {
        if (index_a == 0) {
          start_a = millis();
        }
        LEDOn(LED_RX_A);
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
    ending1 = true;
    index_a = 0;
    start_a = millis();
  }
  if (index_a == 1) {
    dataLength = 3 + (incomingByte & 0x0F);
  }
  if (index_a < MAX_NMEA_BUFFER) {
    buffer_a[index_a] = (byte) incomingByte;
    index_a++;
  }
}

/**
 * my strcat function.
 **/
void strcat(char* original, char appended)
{
  while (*original++)
    ;
  *original--;
  *original = appended;
  *original++;
  *original = '\0';
}

/**
 * writing seatalk datagram.
 **/
inline void writeDatagram() {
  if (index_a == dataLength) {
    dbgOutLn(SEATALK_NMEA_MESSAGE);
    strcpy(linedata,SEATALK_NMEA_MESSAGE); 

    for (byte i = 0; i < index_a; i++) {
      byte value = buffer_a[i];
      byte c = (value & 0xF0) >> 4;
      strcat(linedata, convertNibble2Hex(c));
      c = value & 0x0F;
      strcat(linedata, convertNibble2Hex(c));
    }
    writeData(start_a, 'A', linedata);
    index_a = 0;    
  }
}

/**
 * processing NMEA data.
 **/
inline void NMEAInputA(int incomingByte) {
  byte in = lowByte(incomingByte);
  if (in == 0x0A) {
    ending1 = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_a[index_a] = (byte) in;
      index_a++;
    }
  }
  if (ending1 || (index_a >= MAX_NMEA_BUFFER)) {
    if (index_a > 0) {
#ifdef debug
      if (index_a >= MAX_NMEA_BUFFER) {
        Serial.print('B');
      }
      Serial.print(F("A:"));
      if (index_a > 6) {
        Serial.write(buffer_a, 6);
      } 
      else {
        Serial.write(buffer_a, index_a);
      }
      Serial.println();
#endif
      if (dataFile.isOpen()) {
        writeLEDOn();
        writeTimeStamp(start_a);
        writeChannelMarker('A');
        dataFile.write(buffer_a, index_a);
        dataFile.println();
        //        writeLEDOff();
      }
      index_a = 0;
    }
  } 
}

/**
 * testing the second serial connection.
 **/
void testSecondSerial() {
  if (secondSerial) {
    outputFreeMem('2');
    ending2 = false;
    while ((mySerial.available()  > 0) && !ending2) {
      int incomingByte = mySerial.read();
      if (incomingByte >= 0) {
        if (index_b == 0) {
          start_b = millis();
        }
        LEDOn(LED_RX_B);
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
    ending2 = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_b[index_b] = (byte) in;
      index_b++;
    }
  }
  if (ending2 || (index_b >= MAX_NMEA_BUFFER)) {
    if (index_b > 0) {
#ifdef debug
      if (index_b >= MAX_NMEA_BUFFER) {
        Serial.print('B');
      }
      Serial.print(F("B:"));
      if (index_b > 6) {
        Serial.write(buffer_b, 6);
      } 
      else {
        Serial.write(buffer_b, index_b);
      }
      Serial.println();
#endif
      if (dataFile.isOpen()) {
        writeLEDOn();
        writeTimeStamp(start_b);
        writeChannelMarker('B');
        dataFile.write(buffer_b, index_b);
        dataFile.println();
        //        writeLEDOff();
      }
      index_b = 0;
    }
  } 
}

/**
 * writing a new logger entry.
 **/
void writeData(unsigned long startTime, char marker, char* data) {
  writeTimeStamp(startTime);
  writeChannelMarker(marker);
  writeNMEAData(data);
}

/**
 * writing the timestamp.
 **/
void writeTimeStamp(unsigned long time) {
  if (dataFile.isOpen()) {
    word mil = time % 1000;
    word div = time / 1000;

    byte sec = div % 60;
    byte minute = (div / 60) % 60;
    byte hour = (div / 3600) % 24; 

    sprintf_P(timedata, TIMESTAMP, hour, minute, sec, mil); 
    dataFile.print(timedata);
  }
}

/**
 * writing the channel marker.
 **/
void writeChannelMarker(char marker) {
  if (dataFile.isOpen()) {
    dataFile.print(marker);
    dataFile.print(';');
  }
}

/**
 * writing a new NMEA Datagram.
 **/
void writeNMEAData(char* data) {
  writeLEDOn();
  byte crc = 0;
  for(uint8_t i = 0; i < strlen(data); i++) {
    crc ^= data[i]; 
  }
  if (dataFile.isOpen()) {
    dataFile.write('$');
    dataFile.print(data);
    dataFile.write('*');
    if (crc < 16) {
      dataFile.write('0');
    }
    dataFile.println(crc,HEX);
  }
#ifdef debug
  dbgOut('$');
  dbgOut(data);
  dbgOut('*');
  if (crc < 16) {
    dbgOut('0');
  }
  dbgOutLn2(crc,HEX);
#endif
}


