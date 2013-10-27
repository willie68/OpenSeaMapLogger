#include <AltSoftSerial.h>

#define MAX_BUFFER 80

const byte B_RX = 8;
const byte B_TX = 9;
// RX_A LED is PD5
const byte LED_RX_B = 5; // PD5
// Power LED is PD4
const byte LED_RX_A = 4; // PD4

AltSoftSerial mySerial;

boolean error = false;

boolean seatalkActive = false;
byte buffer_a[MAX_BUFFER];
byte buffer_b[MAX_BUFFER];
byte index_a, index_b;

void setup() {
  index_a = 0;
  index_b = 0;

  //--- init outputs and inputs ---
  // pins for LED's  
  pinMode(LED_RX_A, OUTPUT);
  pinMode(LED_RX_B, OUTPUT);
  PORTD &= ~(_BV(LED_RX_A) | _BV(LED_RX_B));

  Serial.begin(4800, SERIAL_8N1);
  Serial.flush();
  delay(100);

  mySerial.begin(4800);

  PORTD &= ~_BV(LED_RX_B);
  PORTD &= ~_BV(LED_RX_A);
}

long lastMillis;
long lastW;

unsigned long now, start_a, start_b;
bool ending, ending1, ending2;

void loop() {
  // reset LED for receiving channel A 
  now = millis();
  if (now > (start_a + 500)) {
    PORTD &= ~_BV(LED_RX_A);
  }

  // reset LED for receiving channel B 
  if (now > (start_b + 500)) {
    PORTD &= ~_BV(LED_RX_B);
  }

  testFirstSerial();
  testSecondSerial();
}

// 1. Schnittstelle abfragen
void testFirstSerial() {
  ending1 = false;
  while ((Serial.available()  > 0) && !ending1) {
    if (Serial.overflow()) {
      PORTD |= _BV(LED_RX_A);
    }
    int incomingByte = Serial.read();
    if (incomingByte >= 0) {
      if (index_a == 0) {
        start_a = now;
      }
      //        PORTD |= _BV(LED_RX_A);
      InputA(incomingByte);
      ending1 = ending;
    }
  }
}


// 2. Schnittstelle abfragen
void testSecondSerial() {
  ending2 = false;
  while ((mySerial.available()  > 0) && !ending2) {
    if (mySerial.overflow()) {
      PORTD |= _BV(LED_RX_B);
    }
    int incomingByte = mySerial.read();
    if (incomingByte >= 0) {
      if (index_b == 0) {
        start_b = now;
      }
      //PORTD |=_BV(LED_RX_B);
      InputB(incomingByte);
      ending2 = ending;
    }
  }
}

inline void InputA(int incomingByte) {
  byte in = lowByte(incomingByte);
  ending = false;
  if (in == 0x0A) {
    ending = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_a[index_a] = (byte) in;
      index_a++;
    }
  }
  if (ending || (index_a >= MAX_BUFFER)) {
    if (index_a > 0) {
      index_a = 0;
    }
  } 
}

inline void InputB(int incomingByte) {
  byte in = lowByte(incomingByte);
  ending = false;
  if (in == 0x0A) {
    ending = true;
  } 
  else {
    if (in != 0x0D) {
      buffer_b[index_b] = (byte) in;
      index_b++;
    }
  }
  if (ending || (index_b >= MAX_BUFFER)) {
    if (index_b > 0) {
      index_b = 0;
    }
  } 
}


