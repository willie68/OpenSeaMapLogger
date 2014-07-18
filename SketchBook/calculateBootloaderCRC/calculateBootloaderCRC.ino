#include <avr/pgmspace.h>
#include <util/crc16.h>

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);  
}

/**
 * calcualting the 16 bit checksum of a part of the flash memory.
 **/
static word CalculateChecksum (word addr, word size) {
  word crc = ~0;
  prog_uint8_t* p = (prog_uint8_t*) addr;
  for (word i = 0; i < size; ++i) {
    crc = _crc16_update(crc, pgm_read_byte(p++));
  }
  return crc;
}

void loop() {
  word crc = CalculateChecksum(0x7E00, 512);
  Serial.print("crc");
  Serial.println(crc, HEX);
}
