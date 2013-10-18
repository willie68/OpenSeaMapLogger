#include <MemoryFree.h>
#include <avr/pgmspace.h>

//#include <SoftwareSerial.h>
const byte NMEA_B_RX = 8;
const byte NMEA_B_TX = 9;

/*
const char* START_MESSAGE = "POSMST,Start NMEA Logger,V 0.0.3";
 const char* GYRO_MESSAGE = "POSMGYR,%i, %i, %i";
 */
#define GYRO_MESSAGE PSTR("POSMGYR,%i, %i, %i") 
#define SEATALK_NMEA_MESSAGE PSTR("POSMSK,")
#define convertNibble2Hex(c)  (c < 10 ? c + '0' : c + 'A' - 10)

//SoftwareSerial mySerial(NMEA_B_RX, NMEA_B_TX);

void strcat(char* original, char appended)
{
    while (*original++)
    ;
    *original--;
    *original = appended;
    *original++;
    *original = '\0';
}

void setup() {
  // put your setup code here, to run once:
  int16_t ax = 1234;
  int16_t ay = 23456;
  int16_t az = 3456;

  Serial.begin(4800, SERIAL_8N1);
  outputFree();
  Serial.println(F("OpenSeaMap Datalogger"));
  outputFree();
  char data[80];
  strcpy_P(data, PSTR("POSMST,Start NMEA Logger,V 0.0.3"));
  Serial.println(data);
  sprintf_P(data, GYRO_MESSAGE, ax, ay, az); 
  Serial.println(data);
  outputFree();

  strcpy_P(data,SEATALK_NMEA_MESSAGE); 
  Serial.println(data);

  char t[20];
  byte value = 0x1D;
  byte v = (value & 0xF0) >> 4;
  char c = convertNibble2Hex(v);
  strcat(data, c);
  v = value & 0x0F;
  c = convertNibble2Hex(v);
  strcat(data, c);
  Serial.println(data);

  //  mySerial.begin(4800);
  //  mySerial.println("Hello");

  outputFree();
}

void outputFree() {
  Serial.print("RAM:");
  Serial.println(freeMemory());
}

void loop() {
  /*  unsigned long time = millis();
   word mil = time % 1000;
   long div = time / 1000;
   char timedata[10];
   sprintf_P(timedata, PSTR("%lu.%03u: "), div, mil); 
   Serial.println(timedata);
   
   byte sec = div % 60;
   byte minute = (div / 60) % 60;
   byte hour = (div / 3600) % 24; 
   char timedata[16];
   sprintf_P(timedata, PSTR("%02d:%02d:%02d.%03u: "), hour, minute, sec, mil); 
   if (dataFile) {
   dataFile.print(timedata);
   }
   */
  if (Serial.available()  > 0) {
    int incomingByte = Serial.read();
    if (incomingByte >= 0) {
      Serial.write(incomingByte);
    }
  }
  /*
  if (mySerial.available()  > 0) {
   int incomingByte = mySerial.read();
   if (incomingByte >= 0) {
   Serial.write(incomingByte);
   }
   }
   */
}


