#define debug
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

const char* START_MESSAGE = "POSMST,Start NMEA Logger,V 0.0.3";
const char* STOP_MESSAGE = "POSMSO,Stop NMEA Logger";
const char* VCC_MESSAGE = "POSMVCC, %i";
const char* GYRO_MESSAGE = "POSMGYR,%i, %i, %i";
const char* ACC_MESSAGE = "POSMACC,%i, %i, %i";

boolean firstSerial = true;
boolean secondSerial = true;

// Port for NMEA B, NMEA A is the normal Serial connection
SoftwareSerial mySerial(NMEA_B_RX, NMEA_B_TX);

boolean error = false;

MPU6050 accelgyro (MPU6050_ADDRESS_AD0_LOW);

const byte MAX_NMEA_BUFFER = 80;

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

  //--- init outputs and inputs ---
  // pins for LED's  
  dbgOutLn(F("Init Ports"));
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_WRITE, OUTPUT);
  pinMode(LED_RX_A, OUTPUT);
  pinMode(LED_RX_B, OUTPUT);

  // pins for switches 
  pinMode(SW_STOP, INPUT_PULLUP);

  //--- init sd card ---
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SD_CHIPSELECT, OUTPUT);

  // see if the card is present and can be initialized:
  while (!SD.begin(SD_CHIPSELECT)) {
    dbgOutLn(F("Card failed, or not present"));
    PORTD ^= _BV(LED_WRITE);
    //digitalWrite(LED_WRITE, !digitalRead(LED_WRITE));
    delay(500);
  }

  dbgOutLn(F("Init NMEA"));
  if (!getParameters()) {
    //--- init NMEA ports ---
    // NMEA A
    if (firstSerial) {
      Serial.end();
      Serial.begin(4800, SERIAL_8N1);
    }

    if (secondSerial) {
      // NMEA B
      mySerial.begin(4800);
    }
  }

  //--- init MPU6050 ---
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
              dbgOut("I SK:");
            } 
            else {
              dbgOut("I NA:");
            }
            Serial.println(baud, HEX);
#endif
            if (baud > 0) {
              firstSerial = true;
              Serial.end();
              if (seatalkActive) {
                // for seatalk we need another initialisation 
                Serial.begin(baud, SERIAL_8E1);
              }
              else {
                Serial.begin(baud, SERIAL_8N1);
              }              
            }
          }
          if (paramCount == 2) {
            long baud = BAUDRATES[readValue - '0'];
#ifdef debug
            dbgOut("I NB:");
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

inline void writeLEDOn()  {
  lastW = millis();
  PORTD |= _BV(LED_WRITE);
  //  digitalWrite(LED_WRITE,1);
}

inline void writeLEDOff() {
  PORTD &= ~_BV(LED_WRITE);
  //  digitalWrite(LED_WRITE,0);
}


void loop() {
  // im Fehlerfall blinken
  if (error) {
    if ((millis() % 500) > 250) {
      PORTD &= ~_BV(LED_POWER);
    } 
    else {
      PORTD |= _BV(LED_POWER);
    }
    //    digitalWrite(LED_POWER, (millis() % 500) > 250);
  } 
  else {
    PORTD |= _BV(LED_POWER);
    //    digitalWrite(LED_POWER, 1);
  }

  long now = millis();
  if (now > (lastW + 500)) {
    PORTD &= ~_BV(LED_WRITE);    
    //    digitalWrite(LED_WRITE, 0);
  }

  if (now > (lastA + 500)) {
    PORTD &= ~_BV(LED_RX_A);
    //    digitalWrite(LED_RX_A, 0);
  }

  if (now > (lastB + 500)) {
    PORTD &= ~_BV(LED_RX_B);
    //    digitalWrite(LED_RX_B, 0) ;
  }

  // Versorgungsspannung messen
  vcc = readVcc();
  vccTime = millis();
  // Bei Unterspannung, alles herrunter fahren
  if ((vcc < VCC_GOLDCAP) || (digitalRead(SW_STOP)==0)) {
    // Alle LED's aus, Strom sparen
    PORTD &= ~(_BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE));
    //    digitalWrite(LED_POWER, 0);
    //    digitalWrite(LED_RX_A, 0);
    //    digitalWrite(LED_RX_B, 0);
    //    digitalWrite(LED_WRITE, 0);

    if (dataFile) {
      writeVCC();
      stopLogger();
      dbgOutLn(F("Shutdown detected, datafile closed"));
    }
    // Jetzt alle LED's einschalten, damit ordentlich strom verbraucht wird.
    PORTD |= _BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE);
    //    digitalWrite(LED_POWER, 1);
    //    digitalWrite(LED_RX_A, 1);
    //    digitalWrite(LED_RX_B, 1);
    //    digitalWrite(LED_WRITE, 1);
    delay(10000);
  } 
  else {
    if (!dataFile) {
      newFile();
    }

    testFirstSerial();
    testSecondSerial();

    // der Gyro wird nur jede Sekunde einmal abgefragt
    long now = millis();
    if ((now - 1000) > lastMillis) {
      lastMillis = now;
      writeGyroData();
#ifdef debug
      writeVCC();
#endif
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

void writeVCC() {
  char data[80];
  sprintf(data, VCC_MESSAGE, vcc); 
  writeData(vccTime, data);
}

void writeGyroData() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  unsigned long startTime = millis();
  // print to the serial port too:
  if (dataFile) {
    char data[80];
    sprintf(data, GYRO_MESSAGE, ax, ay, az); 
    writeData(startTime, data);
    sprintf(data, ACC_MESSAGE, gx, gy, gz); 
    writeData(startTime, data);
  }  
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
  if (dataFile) {
    char data[80];
    strcpy(data, START_MESSAGE);
    writeData(millis(), data);          // write data to card
  }
}

void stopLogger() {
  if (dataFile) {
    dbgOutLn(F("close datafile."));
    char data[80];
    strcpy(data, STOP_MESSAGE);
    writeData(millis(), data);  // write data to card
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
        digitalWrite(LED_RX_A , 1);
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

byte seatalkCommand, attribute, dataLength;

void SeaTalkInputA(int incomingByte) {
  switch (index_a) {
  case 0:
    seatalkCommand = incomingByte;
    break;
  case 1:
    attribute = incomingByte;
    dataLength = 3+ (attribute & 0x0F);
    break;
  case 2:
  default:
    break;
  }
  buffer_a[index_a] = (byte) incomingByte;
  index_a++;
  if (index_a >= dataLength) {
    if (dataFile) {
      writeLEDOn();
      writeTimeStamp(start_a, dataFile);
      dataFile.write(buffer_a, index_a-1);
      dataFile.println();
      writeLEDOff();
    }
    index_a = 0;    
  }
}

void NMEAInputA(int incomingByte) {
  if ((incomingByte != 0x0A) && (incomingByte != 0x0D)) {
    buffer_a[index_a] = (byte) incomingByte;
    index_a++;
  }
  if (((incomingByte == 0x0A) || (incomingByte == 0x0D) || (index_a >= MAX_NMEA_BUFFER)) && (index_a > 0)) {
#ifdef debug
    dbgOut(F("A:"));
    Serial.write(buffer_a, index_a);
    Serial.println();
#endif
    if (dataFile) {
      writeLEDOn();
      writeTimeStamp(start_a, dataFile);
      dataFile.write(buffer_a, index_a);
      dataFile.println();
      writeLEDOff();
    }
    index_a = 0;
  } 
}

// 2. Schnittstelle abfragen
void testSecondSerial() {
  if (secondSerial) {
    while (mySerial.available()  > 0) {
      boolean myOverflow = mySerial.overflow();
      dbgOutLn("ovl B");
      int incomingByte = mySerial.read();
      if (incomingByte >= 0) {
        if (index_b == 0) {
          start_b = millis();
        }
        digitalWrite(LED_RX_B , 1);
        lastB = millis();
        if ((incomingByte != 0x0A) && (incomingByte != 0x0D)) {
          buffer_b[index_b] = (byte) incomingByte;
          index_b++;
        }
        if (((incomingByte == 0x0A) || (incomingByte == 0x0D) || (index_b >= MAX_NMEA_BUFFER)) && (index_b > 0)) {
#ifdef debug
          dbgOut(F("B:"));
          Serial.write(buffer_b, index_b);
          Serial.println();
#endif
          if (dataFile) {
            writeLEDOn();
            writeTimeStamp(start_b, dataFile);
            dataFile.write(buffer_b, index_b);
            dataFile.println();
            writeLEDOff();
          }
          index_b = 0;
        } 
      }
    }
  }
}

void writeData(unsigned long startTime, char* data) {
  if (dataFile) {
    writeTimeStamp(startTime, dataFile);
    writeNMEAData(data);
  }
}

void writeTimeStamp(unsigned long time, File dataFile) {
  word mil = time % 1000;
  word div = time / 1000;

  byte sec = div % 60;
  byte minute = (div / 60) % 60;
  byte hour = (div / 3600) % 24; 
  //  char timedata[16];
  //  sprintf_P(timedata, PSTR("%02d:%02d:%02d.%03u: "), hour, minute, sec, mil); 
  if (dataFile) {
    print2Dec(hour, dataFile);
    Serial.print(":");
    print2Dec(minute, dataFile);
    Serial.print(":");
    print2Dec(sec, dataFile);
    Serial.print(".");
    print3Dec(mil, dataFile);
    //  dataFile.print(timedata);
  }
}

void writeNMEAData(char* data) {
  writeLEDOn();
  byte crc = 0;
  for(int i = 0; i < strlen(data); i++) {
    crc ^= data[i]; 
  }
  if (dataFile) {
    dataFile.write("$");
    dataFile.print(data);
    dataFile.write("*");
    if (crc < 16) {
      dataFile.write("0");
    }
    dataFile.println(crc,HEX);
  }
#ifdef debug
  dbgOut("$");
  dbgOut(data);
  dbgOut("*");
  dbgOutLn2(crc,HEX);
#endif
  writeLEDOff();
}


