#include <avr/eeprom.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

template <class T> int EEPROM_writeStruct(int address, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
		  eeprom_write_byte((unsigned char *)address++, *p++);  
    return i;
}

template <class T> int EEPROM_readStruct(int address, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = eeprom_read_byte((unsigned char *)address++);
    return i;
}
