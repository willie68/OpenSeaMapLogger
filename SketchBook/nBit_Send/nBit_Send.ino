
const byte LED = 13;

void setup() {
  Serial.begin(4800, SERIAL_9N1);
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, 1);
  for (int i = 0; i < 511; i++)  {
    Serial.write(i);
    delay(50);
    digitalWrite(LED, i > 255);
  }
}
