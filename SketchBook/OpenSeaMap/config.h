#include <Arduino.h>
#include <inttypes.h>
// Port definition

// SD Card
const byte SD_CHIPSELECT = 10;
// NMEA Baudrates
const int BAUDRATES[] = {
  0, 1200, 2400, 4800, 9600, 19200, 38400
};
// NMEA 0183 Port B
const byte NMEA_B_RX = 8;
const byte NMEA_B_TX = 9;

// Activeating 3V3 Supply
const byte SUPPLY_3V3 = 2;

// LEDs
// Write led is PD6
const byte LED_WRITE = 6; // PD6
// Power LED is PD7
const byte LED_POWER = 7; // PD7
// RX_A LED is PD5
const byte LED_RX_B = 5; // PD5
// Power LED is PD4
const byte LED_RX_A = 4; // PD4

// Switches
// Switch stop is PD3
const byte SW_STOP = 3; // PD3

// Voltagevalue for shutdown
const int VCC_GOLDCAP = 200;

const long GOLDCAP_LOADING_TIME = 30000L;
// sopme debug strings
const String vccString = "VCC:";

const char CONFIG_FILE[] = "config.dat";

// EEPROM storage positions
const word EEPROM_BAUD_A = 0x0010;
const word EEPROM_BAUD_B = 0x0011;
const word EEPROM_SEATALK = 0x0012;
const word EEPROM_OUTPUT = 0x0013;
const word EEPROM_VESSELID = 0x0014;// (-17) 4 bytes
const word EEPROM_BOOTLOADER_VERSION = 0x0019;// 1 byte

const word EEPROM_VERSION = E2END - 2;

// constants of the differet bootloader versions
const word BOOTLOADER_2_CONST = 0xB7FD;

