//#define outputVcc
#define outputGyro
#define debug
//#define simulation

#ifdef debug
#include <MemoryFree.h>
#endif
#include <Wire.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include "osm_debug.h"
#include "osm_makros.h"
#include "config.h"
#include <SoftwareSerial.h>
#include <SD.h>

/*
 OpenSeaMap.ino - Logger for the OpenSeaMap - Version 0.0.3
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
 4 = 9600 Baud
 5 = 19200 Baud
 6 = 38400 Baud
 
 for the first serial you can activate the SEATALK Protokoll, which has an other format. 
 If you add an s before the baud value, seatalk protokoll will be activated.
 
 To Load firmware to OSM Lodder rename hex file to OSMFIRMW.HEX and put it on a FAT16 formatted SD card. 
 */
// WKLA 20131010
// - Prüfsumme für Daten eingebaut.

#define START_MESSAGE PSTR("POSMST,Start NMEA Logger,V 0.0.3")
#define STOP_MESSAGE PSTR("POSMSO,Stop NMEA Logger")
#define VCC_MESSAGE PSTR("POSMVCC, %i")
#define GYRO_MESSAGE PSTR("POSMGYR,%i, %i, %i")
#define ACC_MESSAGE PSTR("POSMACC,%i, %i, %i")
#define TIMESTAMP PSTR("%02d:%02d:%02d.%03u: ")
#define SEATALK_NMEA_MESSAGE PSTR("POSMSK,")
#define MAX_NMEA_BUFFER 80

#ifdef debug
#define outputFreeMem() \
  dbgOut(F("RAM:")); \
  dbgOutLn(freeMemory());
#else
#define outputFreeMem()
#endif

boolean firstSerial = true;
boolean secondSerial = true;

// Port for NMEA B, NMEA A is the normal Serial connection
SoftwareSerial mySerial(NMEA_B_RX, NMEA_B_TX);

boolean error = false;

MPU6050 accelgyro (MPU6050_ADDRESS_AD0_LOW);

boolean seatalkActive = false;
byte buffer_a[MAX_NMEA_BUFFER];
byte buffer_b[MAX_NMEA_BUFFER];
byte index_a, index_b;

void setup() {

  index_a = 0;
  index_b = 0;

  initDebug();

  // prints title with ending line break
  dbgOutLn(F("OpenSeaMap Datalogger"));
  outputFreeMem();

  //--- init outputs and inputs ---
  // pins for LED's  
  dbgOutLn(F("Init Ports"));
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_WRITE, OUTPUT);
  pinMode(LED_RX_A, OUTPUT);
  pinMode(LED_RX_B, OUTPUT);
  PORTD &= ~(_BV(LED_WRITE) | _BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B));

  // pins for switches 
  pinMode(SW_STOP, INPUT_PULLUP);

  //--- init sd card ---
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SD_CHIPSELECT, OUTPUT);

  PORTD ^= _BV(LED_POWER);

  // see if the card is present and can be initialized:
  dbgOutLn(F("checking SD Card"));
#ifndef simulation
  while (!SD.begin(SD_CHIPSELECT)) {
    dbgOutLn(F("Card failed, or not present"));
    PORTD ^= _BV(LED_WRITE);
    outputFreeMem();
    delay(500);
  }
#endif
  dbgOutLn(F("SD Card ready"));

  PORTD ^= _BV(LED_RX_A);
  dbgOutLn(F("Init NMEA"));
  if (!getParameters()) {
    //--- init NMEA ports ---
    // NMEA A
    if (firstSerial) {
#ifdef debug
      Serial.flush();
      Serial.end();
#endif
      Serial.begin(4800, SERIAL_8N1);
      Serial.flush();
      delay(100);
    }

    if (secondSerial) {
      // NMEA B
      mySerial.begin(4800);
    }
  }

  //--- init MPU6050 ---
  // join I2C bus (I2Cdev library doesn't do this automatically)
  dbgOutLn(F("Init I2C"));
  PORTD ^= _BV(LED_RX_B);
  Wire.begin();

  // initialize device
  accelgyro.initialize();

  if (accelgyro.testConnection()) {
    dbgOutLn(F("MPU6050 successful"));
    accelgyro.setFullScaleAccelRange(0);
    accelgyro.setFullScaleGyroRange(0);    
  } 
  else {
    PORTD &= ~_BV(LED_RX_B);
    dbgOutLn(F("MPU6050 failed"));
  }
}

boolean getParameters() {
  dbgOutLn(F("readconf"));
  boolean result = false;
  char filename[11] = "config.dat";
  if (SD.exists(filename)) {    // only open a new file if it doesn't exist
    result = true;
    File configFile = SD.open(filename);
    if (configFile) {
      byte paramCount = 0;
      boolean lastCR = false;
      firstSerial = false;
      secondSerial = false;
      while (configFile.available()) {
        byte readValue = configFile.read();
        if (readValue == 's') {
          seatalkActive = true;
          readValue = configFile.read();
        }
        if ((readValue == 0x13) || (readValue == 0x10)) {
          if (!lastCR) {
            paramCount++;
            lastCR = true;
          }
        } 
        else {
          lastCR = false;
          if (paramCount == 1) {
            long baud = BAUDRATES[readValue - '0'];
#ifdef debug
            if (seatalkActive) {
              dbgOut(F("I SK:"));
            } 
            else {
              dbgOut(F("I NA:"));
            }
            Serial.println(baud, HEX);
#endif
            if (baud > 0) {
              firstSerial = true;
              Serial.end();
              if (seatalkActive) {
                // for seatalk we need another initialisation 
                Serial.begin(baud, SERIAL_8N1);
              }
              else {
                Serial.begin(baud, SERIAL_8N1);
              }              
            }
          }
          if (paramCount == 2) {
            long baud = BAUDRATES[readValue - '0'];
#ifdef debug
            dbgOut(F("I NB:"));
            Serial.println(baud, HEX);
#endif
            if (baud > 0) {
              secondSerial = true;
              mySerial.end();
              mySerial.begin(baud);
            }
          }
        }
      }
      configFile.close();
    }
  }
  return result;
}

File dataFile;
long count = 0;
int filecount = 0;

char filename[] = "data0000.dat";

int16_t ax, ay, az;
int16_t gx, gy, gz;

long vcc;
long lastMillis;
long lastW;
long lastA;
long lastB;

unsigned long vccTime, start_a, start_b;
char linedata[MAX_NMEA_BUFFER];
char timedata[15];

inline void writeLEDOn()  {
  lastW = millis();
  PORTD |= _BV(LED_WRITE);
}

inline void writeLEDOff() {
  PORTD &= ~_BV(LED_WRITE);
}

void loop() {
  // im Fehlerfall blinken
  if (error) {
    dbgOutLn(F("ERROR"));
    if ((millis() % 500) > 250) {
      PORTD &= ~_BV(LED_POWER);
    } 
    else {
      PORTD |= _BV(LED_POWER);
    }
  } 
  else {
    PORTD |= _BV(LED_POWER);
  }

  long now = millis();
  if (now > (lastW + 500)) {
    PORTD &= ~_BV(LED_WRITE);    
  }

  if (now > (lastA + 500)) {
    PORTD &= ~_BV(LED_RX_A);
  }

  if (now > (lastB + 500)) {
    PORTD &= ~_BV(LED_RX_B);
  }

  // Versorgungsspannung messen
  vcc = readVcc();
  vccTime = millis();
  // Bei Unterspannung, alles herrunter fahren
  if ((vcc < VCC_GOLDCAP) || (digitalRead(SW_STOP)==0)) {
    // Alle LED's aus, Strom sparen
    PORTD &= ~(_BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE));

    if (dataFile) {
      writeVCC();
      stopLogger();
      dbgOutLn(F("Shutdown detected, datafile closed"));
    }
    // Jetzt alle LED's einschalten, damit ordentlich strom verbraucht wird.
    PORTD |= _BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE);
    delay(10000);
  } 
  else {
#ifndef simulation
    if (!dataFile) {
      newFile();
    }
#endif

    testFirstSerial();
    testSecondSerial();

    // der Gyro wird nur jede Sekunde einmal abgefragt
    long now = millis();
    if ((now - 1000) > lastMillis) {
      lastMillis = now;
#ifdef outputGyro
      writeGyroData();
#endif      
      writeVCC();
    }
  }
}

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

#ifdef outputVcc
inline void writeVCC() {
  sprintf_P(linedata, VCC_MESSAGE, vcc); 
  writeData(vccTime, linedata);
}
#else 
inline void writeVCC() {
}
#endif

inline void writeGyroData() {
#ifdef simulation
  ax = 1234;
  ay = 2345;
  az = 3456;
  gx = 4567;
  gy = 5678;
  gz = 6789;
#else
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
#endif
  unsigned long startTime = millis();
  // print to the serial port too:
  sprintf_P(linedata, GYRO_MESSAGE, ax, ay, az); 
  writeData(startTime, linedata);
  sprintf_P(linedata, ACC_MESSAGE, gx, gy, gz); 
  writeData(startTime, linedata);
}

word lastStartNumber = 0;
// creating a new file with a new filename on the sd card.
void newFile() {
  stopLogger();
  int t = 0; 
  int h = 0;
  int z = 0;
  int e = 0;
  for (word i = lastStartNumber; i < 10000; i++) { // create new filename w/ 3 digits 000-999
    t = i/1000;
    filename[4] = t + '0';          // calculates tausends position
    h = (i - (t*1000)) / 100;
    filename[5] = h + '0';          // calculates hundred position
    z = (i - (t*1000) - (h*100)) / 10 ;
    filename[6] = z + '0';          // subtracts hundreds & tens for single digit
    e = i - (t*1000) - (h*100) - (z*10);
    filename[7] = e + '0';          // calculates hundreds position
    if (! SD.exists(filename)) {    // only open a new file if it doesn't exist
      dataFile = SD.open(filename, FILE_WRITE);
      lastStartNumber = i;
      break;                        // leave the loop after finding new filename
    }
  }
  strcpy_P(linedata, START_MESSAGE);
  dbgOutLn(F("Start"));
  writeData(millis(), linedata);          // write data to card
}

void stopLogger() {
  dbgOutLn(F("close datafile."));
  strcpy_P(linedata, STOP_MESSAGE);
  writeData(millis(), linedata);  // write data to card
  if (dataFile) {
    dataFile.flush();
    dataFile.close();
  }
}

// 1. Schnittstelle abfragen
void testFirstSerial() {
  if (firstSerial) {
    while (Serial.available()  > 0) {
      int incomingByte = Serial.read();
      if (incomingByte >= 0) {
        if (index_a == 0) {
          start_a = millis();
        }
        PORTD |= _BV(LED_RX_A);
        lastA = millis();
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

byte dataLength;

void SeaTalkInputA(int incomingByte) {
  if ((incomingByte & 0x0100) > 0) {
    writeDatagramm();
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

void strcat(char* original, char appended)
{
    while (*original++)
    ;
    *original--;
    *original = appended;
    *original++;
    *original = '\0';
}

inline void writeDatagramm() {
  if (index_a == dataLength) {
/*
    if (dataFile) {
      writeLEDOn();
      writeTimeStamp(start_a);
*/
      dbgOutLn(SEATALK_NMEA_MESSAGE);
      strcpy(linedata,SEATALK_NMEA_MESSAGE); 

      for (byte i = 0; i < index_a; i++) {
        byte value = buffer_a[i];
        byte c = (value & 0xF0) >> 4;
        strcat(linedata, convertNibble2Hex(c));
        c = value & 0x0F;
        strcat(linedata, convertNibble2Hex(c));
      }
      writeData(start_a, linedata);

/*
      dataFile.print(SEATALK_NMEA_MESSAGE);
      for (byte i = 0; i < index_a; i++) {
        byte value = buffer_a[i];
        byte c = (value & 0xF0) >> 4;
        dataFile.write(convertNibble2Hex(c));
        c = value & 0x0F;
        dataFile.write(convertNibble2Hex(c));
      }
      dataFile.println();
      writeLEDOff();
    }
*/
    index_a = 0;    
  }
}

void NMEAInputA(int incomingByte) {
  byte in = lowByte(incomingByte);
  boolean ending = false;
  if (in == 0x0A) {
    ending = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_a[index_a] = (byte) in;
      index_a++;
    }
  }
  if (ending || (index_a >= MAX_NMEA_BUFFER)) {
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
      if (dataFile) {
        writeLEDOn();
        writeTimeStamp(start_a);
        dataFile.write(buffer_a, index_a);
        dataFile.println();
        writeLEDOff();
      }
      index_a = 0;
    }
  } 
}

// 2. Schnittstelle abfragen
void testSecondSerial() {
  if (secondSerial) {
    while (mySerial.available()  > 0) {
      boolean myOverflow = mySerial.overflow();
      dbgOutLn(F("ovl B"));
      int incomingByte = mySerial.read();
      if (incomingByte >= 0) {
        if (index_b == 0) {
          start_b = millis();
        }
        PORTD |=_BV(LED_RX_B);
        lastB = millis();
        NMEAInputB(incomingByte);
      }
    }
  }
}

void NMEAInputB(int incomingByte) {
  byte in = lowByte(incomingByte);
  boolean ending = false;
  if (in == 0x0A) {
    ending = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_b[index_b] = (byte) in;
      index_b++;
    }
  }
  if (ending || (index_b >= MAX_NMEA_BUFFER)) {
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
      if (dataFile) {
        writeLEDOn();
        writeTimeStamp(start_b);
        dataFile.write(buffer_b, index_b);
        dataFile.println();
        writeLEDOff();
      }
      index_b = 0;
    }
  } 
}

void writeData(unsigned long startTime, char* data) {
  writeTimeStamp(startTime);
  writeNMEAData(data);
}

void writeTimeStamp(unsigned long time) {
  if (dataFile) {
    word mil = time % 1000;
    word div = time / 1000;

    byte sec = div % 60;
    byte minute = (div / 60) % 60;
    byte hour = (div / 3600) % 24; 

    sprintf_P(timedata, TIMESTAMP, hour, minute, sec, mil); 
    dataFile.print(timedata);
  }
}

void writeNMEAData(char* data) {
  writeLEDOn();
  byte crc = 0;
  for(uint8_t i = 0; i < strlen(data); i++) {
    crc ^= data[i]; 
  }
  if (dataFile) {
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
  writeLEDOff();
}

