#include <SoftwareSerial.h>
#include <MemoryFree.h>
#include <avr/pgmspace.h>

// NMEA 0183 Port B
const byte NMEA_B_RX = 2;
const byte NMEA_B_TX = 3;
const byte BOARD_LED = 13;

// Port for NMEA B, NMEA A is the normal Serial connection
SoftwareSerial mySerial(NMEA_B_RX, NMEA_B_TX);

void setup() {
  Serial.begin(4800, SERIAL_9N1);
  outputFree();
  
  mySerial.begin(4800);
}

void outputFree() {
  mySerial.print("RAM:");
  mySerial.println(freeMemory());
}

boolean command = false;

void loop() {
  digitalWrite(BOARD_LED, (millis() % 1000) > 500);
  if (mySerial.available()  > 0) {
    int incomingByte = mySerial.read();
    mySerial.write(incomingByte);
    if (incomingByte >= 0) {
      if (incomingByte == 'C') {
        command = true;
        outputFree();
      } else {
        if (command) {
          incomingByte |= 0x0100;
        }
        Serial.write(incomingByte);
        command = false;
      }
    }
  } else {
        Serial.write(0x0155);
        delay(100);
  }
//  outputFree();
}


