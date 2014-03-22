/*
  makros.cpp - Ein paar nützliche Makros - Version 0.1
  Copyright (c) 2012 Wilfried Klaas.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define between(a,x,y) ((a >=x) && (a <= y))

#define convertNibble2Hex(c)  (c < 10 ? c + '0' : c + 'A' - 10)

#define writeLEDOn() \
  lastW = millis(); \
  PORTD |= _BV(LED_WRITE);

#define LEDOn(led) \
  PORTD |= _BV(led);

#define LEDOff(led) \
  PORTD &= ~_BV(led);

#define LEDBlink(led) \
  PORTD ^= _BV(led);
  
#define LEDAllOff() \
  PORTD &= ~(_BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE));

#define LEDAllOn() \
  PORTD |= _BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE); 
  
#define LEDAllBlink() \
  PORTD ^= (_BV(LED_POWER) | _BV(LED_RX_A) | _BV(LED_RX_B) | _BV(LED_WRITE));
  
#ifdef freemem
#define outputFreeMem(s) \
  Serial.print(s); \
  Serial.print(F(":RAM:")); \
  Serial.println(freeMemory());
#else
#define outputFreeMem(s)
#endif


