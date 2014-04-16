#include <Arduino.h>
// Port definition

// SD Card
const byte SD_CHIPSELECT = 10;
// NMEA Baudrates
const int BAUDRATES[] = {
  0, 1200, 2400, 4800, 9600, 19200, 38400
};

// Activeating 3V3 Supply
const byte SUPPLY_3V3 = 2;

// NMEA 0183 Port B
const byte NMEA_B_RX = 8;
const byte NMEA_B_TX = 9;
// LEDs
// Write led is PD6
const byte LED_WRITE = 6;
// Power LED is PD7
const byte LED_POWER = 7;
// RX_A LED is PD5
const byte LED_RX_B = 5;
// Power LED is PD4
const byte LED_RX_A = 4;

// Switches
// Switch stop is PD3
const byte SW_STOP = 3;

// Voltagevalue for shutdown
const int VCC_GOLDCAP = 4600;

// sopme debug strings
const String vccString = "VCC:";

const char CONFIG_FILE[] = "config.dat";

const word EEPROM_VERSION = E2END - 2;

