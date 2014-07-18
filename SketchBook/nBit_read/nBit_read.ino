#include <AltSoftSerial.h>

const byte LED = 13;
AltSoftSerial altSerial;

void setup() {
  altSerial.begin(9600);
  altSerial.println("Hello World");

  Serial.begin(4800, SERIAL_9N1);
  pinMode(LED, OUTPUT);
}

int i = 0;
void loop() {
  while(Serial.available()) {
    int value = Serial.read();
    i++;
    if ((value == 170) || (value == 341)) {
      digitalWrite(LED, i%2);
    } else {
      digitalWrite(LED, 0);
    }
    altSerial.print(value, BIN);
    altSerial.print(',');
    altSerial.println(value);
  }
}
