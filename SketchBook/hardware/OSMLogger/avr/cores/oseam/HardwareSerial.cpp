/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
  Modified 14 August 2012 by Alarus
  Modified 14 October 2013 by Wilfried Klaas
  - separate buffer sizes for input/output
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "wiring_private.h"

// this next line disables the entire HardwareSerial.cpp, 
// this is so I can support Attiny series and any other chip without a uart
#if defined(UBRRH) || defined(UBRR0H) || defined(UBRR1H) || defined(UBRR2H) || defined(UBRR3H)

#include "HardwareSerial.h"

/*
 * on ATmega8, the uart and its bits are not numbered, so there is no "TXC0"
 * definition.
 */
#if !defined(TXC0)
#if defined(TXC)
#define TXC0 TXC
#elif defined(TXC1)
// Some devices have uart1 but no uart0
#define TXC0 TXC1
#else
#error TXC0 not definable in HardwareSerial.h
#endif
#endif

#define getOffset(position) position % 8;
#define getIndex(position) position / 8;


struct ring_buffer
{
  unsigned char rx_buffer[SERIAL_RX_BUFFER_SIZE];
  unsigned char nrx_buffer[SERIAL_NRX_BUFFER_SIZE];
  volatile unsigned int rx_head;
  volatile unsigned int rx_tail;

  unsigned char tx_buffer[SERIAL_TX_BUFFER_SIZE];
  unsigned char ntx_buffer[SERIAL_NTX_BUFFER_SIZE];
  volatile unsigned int tx_head;
  volatile unsigned int tx_tail;
  
  volatile bool overflow;
};

#if defined(USBCON)
  ring_buffer buffer = { { 0 }, { 0 }, 0, 0, { 0 }, { 0 }, 0, 0, false};
#endif
#if defined(UBRRH) || defined(UBRR0H)
  ring_buffer buffer = { { 0 }, { 0 }, 0, 0, { 0 }, { 0 }, 0, 0, false};
#endif
#if defined(UBRR1H)
  ring_buffer buffer1 = { { 0 }, { 0 }, 0, 0, { 0 }, { 0 }, 0, 0, false};
#endif
#if defined(UBRR2H)
  ring_buffer buffer2 = { { 0 }, { 0 }, 0, 0, { 0 }, { 0 }, 0, 0, false};
#endif
#if defined(UBRR3H)
  ring_buffer buffer3 = { { 0 }, { 0 }, 0, 0, { 0 }, { 0 }, 0, 0, false};
#endif

inline void store_char(unsigned char c, unsigned char nb, ring_buffer *buffer)
{
  unsigned int i = (unsigned int)(buffer->rx_head + 1) % SERIAL_RX_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != buffer->rx_tail) {
    unsigned char index = getIndex(buffer->rx_head);
    unsigned char offset = getOffset(buffer->rx_head);
    buffer->rx_buffer[buffer->rx_head] = c;
	if (nb > 0) {
      buffer->nrx_buffer[index] |= _BV(offset); 
	} else {
      buffer->nrx_buffer[index] &= ~_BV(offset);
	}
	buffer->rx_head = i;
  } else {
    buffer->overflow = true;
  }
}

#if !defined(USART0_RX_vect) && defined(USART1_RX_vect)
// do nothing - on the 32u4 the first USART is USART1
#else
#if !defined(USART_RX_vect) && !defined(USART0_RX_vect) && \
    !defined(USART_RXC_vect)
  #error "Don't know what the Data Received vector is called for the first UART"
#else
  void serialEvent() __attribute__((weak));
  void serialEvent() {}
  #define serialEvent_implemented
#if defined(USART_RX_vect)
  ISR(USART_RX_vect)
#elif defined(USART0_RX_vect)
  ISR(USART0_RX_vect)
#elif defined(USART_RXC_vect)
  ISR(USART_RXC_vect) // ATmega8
#endif
  {
  #if defined(UDR0)
    if (bit_is_clear(UCSR0A, UPE0)) {
	  unsigned char nb = UCSR0B & 0x02;
	  unsigned char c = UDR0;
      store_char(c, nb, &buffer);
    } else {
      unsigned char c = UDR0;
    };
  #elif defined(UDR)
    if (bit_is_clear(UCSRA, PE)) {
	  unsigned char nb = UCSRB & 0x02;
      unsigned char c = UDR;
      store_char(c, nb, &buffer);
    } else {
      unsigned char c = UDR;
    };
  #else
    #error UDR not defined
  #endif
  }
#endif
#endif

#if defined(USART1_RX_vect)
  void serialEvent1() __attribute__((weak));
  void serialEvent1() {}
  #define serialEvent1_implemented
  ISR(USART1_RX_vect)
  {
    if (bit_is_clear(UCSR1A, UPE1)) {
      unsigned char nb = UCSR1B & 0x02;
      unsigned char c = UDR1;
      store_char(c, nb, &buffer1);
    } else {
      unsigned char c = UDR1;
    };
  }
#endif

#if defined(USART2_RX_vect) && defined(UDR2)
  void serialEvent2() __attribute__((weak));
  void serialEvent2() {}
  #define serialEvent2_implemented
  ISR(USART2_RX_vect)
  {
    if (bit_is_clear(UCSR2A, UPE2)) {
	  unsigned char nb = UCSR2B & 0x02;
      unsigned char c = UDR2;
      store_char(c, nb, &buffer2);
    } else {
      unsigned char c = UDR2;
    };
  }
#endif

#if defined(USART3_RX_vect) && defined(UDR3)
  void serialEvent3() __attribute__((weak));
  void serialEvent3() {}
  #define serialEvent3_implemented
  ISR(USART3_RX_vect)
  {
    if (bit_is_clear(UCSR3A, UPE3)) {
	  unsigned char nb = UCSR3B & 0x02;
      unsigned char c = UDR3;
      store_char(c, nb, &buffer3);
    } else {
      unsigned char c = UDR3;
    };
  }
#endif

void serialEventRun(void)
{
#ifdef serialEvent_implemented
  if (Serial.available()) serialEvent();
#endif
#ifdef serialEvent1_implemented
  if (Serial1.available()) serialEvent1();
#endif
#ifdef serialEvent2_implemented
  if (Serial2.available()) serialEvent2();
#endif
#ifdef serialEvent3_implemented
  if (Serial3.available()) serialEvent3();
#endif
}


#if !defined(USART0_UDRE_vect) && defined(USART1_UDRE_vect)
// do nothing - on the 32u4 the first USART is USART1
#else
#if !defined(UART0_UDRE_vect) && !defined(UART_UDRE_vect) && !defined(USART0_UDRE_vect) && !defined(USART_UDRE_vect)
  #error "Don't know what the Data Register Empty vector is called for the first UART"
#else
#if defined(UART0_UDRE_vect)
ISR(UART0_UDRE_vect)
#elif defined(UART_UDRE_vect)
ISR(UART_UDRE_vect)
#elif defined(USART0_UDRE_vect)
ISR(USART0_UDRE_vect)
#elif defined(USART_UDRE_vect)
ISR(USART_UDRE_vect)
#endif
{
  if (buffer.tx_head == buffer.tx_tail) {
	// Buffer empty, so disable interrupts
#if defined(UCSR0B)
    cbi(UCSR0B, UDRIE0);
#else
    cbi(UCSRB, UDRIE);
#endif
  }
  else {
    // There is more data in the output buffer. Send the next byte
    unsigned char index = getIndex(buffer.tx_tail);
    unsigned char offset = getOffset(buffer.tx_tail);
	unsigned char nb = buffer.ntx_buffer[index] & _BV(offset);
    unsigned char c = buffer.tx_buffer[buffer.tx_tail];
    buffer.tx_tail = (buffer.tx_tail + 1) % SERIAL_TX_BUFFER_SIZE;
	
  #if defined(UDR0)
    UCSR0B &= ~(1<<TXB80);
    if ( nb > 0 )
      UCSR0B |= (1<<TXB80);
    UDR0 = c;
  #elif defined(UDR)
    UCSRB &= ~(1<<TXB8);
    if ( nb > 0)
      UCSRB |= (1<<TXB8);
    UDR = c;
  #else
    #error UDR not defined
  #endif
  }
}
#endif
#endif

#ifdef USART1_UDRE_vect
ISR(USART1_UDRE_vect)
{
  if (buffer1.tx_head == buffer1.tx_tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR1B, UDRIE1);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    unsigned char index = getIndex(buffer1.tx_tail);
    unsigned char offset = getOffset(buffer1.tx_tail);

	unsigned char nb = buffer1.ntx_buffer[index] & _BV(offset);
    unsigned char c = buffer1.tx_buffer[buffer1.tx_tail];
    buffer1.tx_tail = (buffer1.tx_tail + 1) % SERIAL_TX_BUFFER_SIZE;
	
    UCSR1B &= ~(1<<TXB81);
    if ( nb > 0)
      UCSR1B |= (1<<TXB81);
    UDR1 = c;
  }
}
#endif

#ifdef USART2_UDRE_vect
ISR(USART2_UDRE_vect)
{
  if (buffer2.tx_head == buffer2.tx_tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR2B, UDRIE2);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    unsigned char index = getIndex(buffer2.tx_tail);
    unsigned char offset = getOffset(buffer2.tx_tail);
	unsigned char nb = buffer2.ntx_buffer[index] & _BV(offset);
    unsigned char c = buffer2.tx_buffer[buffer2.tx_tail];
    buffer2.tx_tail = (buffer2.tx_tail + 1) % SERIAL_TX_BUFFER_SIZE;
	
    UCSR2B &= ~(1<<TXB82);
    if ( nb > 0)
      UCSR2B |= (1<<TXB82);
    UDR2 = c;
  }
}
#endif

#ifdef USART3_UDRE_vect
ISR(USART3_UDRE_vect)
{
  if (buffer3.tx_head == buffer3.tx_tail) {
	// Buffer empty, so disable interrupts
    cbi(UCSR3B, UDRIE3);
  }
  else {
    // There is more data in the output buffer. Send the next byte
    unsigned char index = getIndex(buffer3.tx_tail);
    unsigned char offset = getOffset(buffer3.tx_tail);
	unsigned char nb = buffer3.ntx_buffer[index] & _BV(offset);
    unsigned char c = buffer3.tx_buffer[buffer3.tx_tail];
    buffer3.tail = (buffer3.tx_tail + 1) % SERIAL_TX_BUFFER_SIZE;
	
    UCSR3B &= ~(1<<TXB83);
    if ( nb > 0)
      UCSR3B |= (1<<TXB83);
    UDR3 = c;
  }
}
#endif


// Constructors ////////////////////////////////////////////////////////////////

HardwareSerial::HardwareSerial(ring_buffer *buffer, 
  volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
  volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
  volatile uint8_t *ucsrc, volatile uint8_t *udr,
  uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t udrie, uint8_t u2x)
{
  _buffer = buffer;
  _ubrrh = ubrrh;
  _ubrrl = ubrrl;
  _ucsra = ucsra;
  _ucsrb = ucsrb;
  _ucsrc = ucsrc;
  _udr = udr;
  _rxen = rxen;
  _txen = txen;
  _rxcie = rxcie;
  _udrie = udrie;
  _u2x = u2x;
  
  initBuffer();
}

// Private Methods //////////////////////////////////////////////////////////////

void HardwareSerial::initBuffer()
{
  for(int i = 0; i < SERIAL_RX_BUFFER_SIZE; i++) 
	_buffer->rx_buffer[i] = 0;
	
  for(int i = 0; i < SERIAL_NRX_BUFFER_SIZE; i++) 
	_buffer->nrx_buffer[i] = 0;

  for(int i = 0; i < SERIAL_TX_BUFFER_SIZE; i++) 
	_buffer->tx_buffer[i] = 0;

  for(int i = 0; i < SERIAL_NTX_BUFFER_SIZE; i++) 
	_buffer->ntx_buffer[i] = 0;

  _buffer->rx_head = 0;
  _buffer->rx_tail = 0;
  _buffer->tx_head = 0;
  _buffer->tx_tail = 0;
  _buffer->overflow = false;
  
  }

// Public Methods //////////////////////////////////////////////////////////////

void HardwareSerial::begin(unsigned long baud, byte config)
{
  uint16_t baud_setting;
  //uint8_t current_config;
  bool use_u2x = true;
  _nineBitMode = config & 0x01;

#if F_CPU == 16000000UL
  // hardcoded exception for compatibility with the bootloader shipped
  // with the Duemilanove and previous boards and the firmware on the 8U2
  // on the Uno and Mega 2560.
  if (baud == 57600) {
    use_u2x = false;
  }
#endif

try_again:
  
  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }
  
  if ((baud_setting > 4095) && use_u2x)
  {
    use_u2x = false;
    goto try_again;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  //set the data bits, parity, and stop bits
#if defined(__AVR_ATmega8__)
  config |= 0x80; // select UCSRC register (shared with UBRRH)
#endif
  *_ucsrc = config & 0xFE;

  // setting up 9'th bit
  *_ucsrb &= ~_BV(2);
  if (config & 0x01) 
    *_ucsrb |= _BV(2);

  sbi(*_ucsrb, _rxen);
  sbi(*_ucsrb, _txen);
  sbi(*_ucsrb, _rxcie);
  cbi(*_ucsrb, _udrie);
}

void HardwareSerial::end()
{
  // wait for transmission of outgoing data
  while (_buffer->tx_head != _buffer->tx_tail)
    ;

  cbi(*_ucsrb, _rxen);
  cbi(*_ucsrb, _txen);
  cbi(*_ucsrb, _rxcie);  
  cbi(*_ucsrb, _udrie);
  
  // clear any received data
  _buffer->rx_head = _buffer->rx_tail;
}

int HardwareSerial::available(void)
{
  return (unsigned int)(SERIAL_RX_BUFFER_SIZE + _buffer->rx_head - _buffer->rx_tail) % SERIAL_RX_BUFFER_SIZE;
}

bool HardwareSerial::overflow(void)
{
  return _buffer->overflow;
}

int HardwareSerial::peek(void)
{
  if (_buffer->rx_head == _buffer->rx_tail) {
    return -1;
  } else {
    int c = _buffer->rx_buffer[_buffer->rx_tail];
    if (_nineBitMode) {
      unsigned char index = getIndex(_buffer->rx_tail);
      unsigned char offset = getOffset(_buffer->rx_tail);
	  unsigned char nb = _buffer->nrx_buffer[index] & (1<<(offset));
	  if ( nb > 0) {
	    c |= _BV(8);
	  }
	}
    return c;
  }
}

int HardwareSerial::read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (_buffer->rx_head == _buffer->rx_tail) {
    return -1;
  } else {
    int c = _buffer->rx_buffer[_buffer->rx_tail];
    if (_nineBitMode) {
      unsigned char index = getIndex(_buffer->rx_tail);
      unsigned char offset = getOffset(_buffer->rx_tail);
	  unsigned char nb = _buffer->nrx_buffer[index] & (1<<(offset));
      if ( nb > 0) {
	    c |= _BV(8);
	  }
	}
    _buffer->rx_tail = (unsigned int)(_buffer->rx_tail + 1) % SERIAL_RX_BUFFER_SIZE;
    _buffer->overflow = false;
    return c;
  }
}

void HardwareSerial::flush()
{
  // UDR is kept full while the buffer is not empty, so TXC triggers when EMPTY && SENT
  while (transmitting && ! (*_ucsra & _BV(TXC0)));
  transmitting = false;
}

size_t HardwareSerial::write(int c)
{
  unsigned int i = (_buffer->tx_head + 1) % SERIAL_TX_BUFFER_SIZE;

  // If the output buffer is full, there's nothing for it other than to 
  // wait for the interrupt handler to empty it a bit
  // ???: return 0 here instead?
  while (i == _buffer->tx_tail)
    ;
  if (_nineBitMode) {
    unsigned char index = getIndex(_buffer->tx_head);
    unsigned char offset = getOffset(_buffer->tx_head);

    _buffer->ntx_buffer[index] &= ~_BV(offset);
    if (c & 0x0100) {
      _buffer->ntx_buffer[index] |= _BV(offset);
    }
  }
  _buffer->tx_buffer[_buffer->tx_head] = lowByte(c);
  _buffer->tx_head = i;
	
  sbi(*_ucsrb, _udrie);
  // clear the TXC bit -- "can be cleared by writing a one to its bit location"
  transmitting = true;
  sbi(*_ucsra, TXC0);
  
  return 1;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

#if defined(UBRRH) && defined(UBRRL)
  HardwareSerial Serial(&buffer, &UBRRH, &UBRRL, &UCSRA, &UCSRB, &UCSRC, &UDR, RXEN, TXEN, RXCIE, UDRIE, U2X);
#elif defined(UBRR0H) && defined(UBRR0L)
  HardwareSerial Serial(&buffer, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UCSR0C, &UDR0, RXEN0, TXEN0, RXCIE0, UDRIE0, U2X0);
#elif defined(USBCON)
  // do nothing - Serial object and buffers are initialized in CDC code
#else
  #error no serial port defined  (port 0)
#endif

#if defined(UBRR1H)
  HardwareSerial Serial1(&buffer1, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UCSR1C, &UDR1, RXEN1, TXEN1, RXCIE1, UDRIE1, U2X1);
#endif
#if defined(UBRR2H)
  HardwareSerial Serial2(&buffer2, &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UCSR2C, &UDR2, RXEN2, TXEN2, RXCIE2, UDRIE2, U2X2);
#endif
#if defined(UBRR3H)
  HardwareSerial Serial3(&buffer3, &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UCSR3C, &UDR3, RXEN3, TXEN3, RXCIE3, UDRIE3, U2X3);
#endif

#endif // whole file
