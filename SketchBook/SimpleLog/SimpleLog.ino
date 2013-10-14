//#include <SoftwareSerial.h>
const byte NMEA_B_RX = 8;
const byte NMEA_B_TX = 9;

//SoftwareSerial mySerial(NMEA_B_RX, NMEA_B_TX);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(4800, SERIAL_8N1);
  Serial.println("Hello");

//  mySerial.begin(4800);
//  mySerial.println("Hello");

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



