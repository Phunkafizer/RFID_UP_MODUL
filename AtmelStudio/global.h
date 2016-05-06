/*-----------------------------------------------------------------------------
FILENAME: global.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: hardware definitions
-----------------------------------------------------------------------------*/

#ifndef global_h
#define global_h

#include <avr/io.h>

#define STX	0x02
#define	ETX	0x03
#define ESC 0x1B
#define ESC_ESC 0x1B

extern const uint8_t empty_tag[];

extern bool useexteeprom;

#if (HWVERSION == 3)
	#define RELAISPORT	PORTC
	#define RELAISDDR		DDRC
	#define RELAISPINX	PC0

	#define RSDIRPORT		PORTC
	#define RSDIRDDR		DDRC
	#define RSDIRPINX		PC3

	#define LEDAPORT		PORTC
	#define LEDADDR		DDRC
	#define LEDAPINX		PC2

	#define LEDBPORT		PORTC
	#define LEDBDDR		DDRC
	#define LEDBPINX		PC1

	#define BUTTONPORT	PORTB
	#define BUTTONPIN	PINB
	#define BUTTONDDR	DDRB
	#define BUTTONPINX	PB0
	
	#define SHDPORT		PORTD
	#define SHDDDR		DDRD
	#define SHDPINX		PD5
#endif

#if (HWVERSION == 2)
	#define RELAISPORT	PORTC
	#define RELAISDDR		DDRC
	#define RELAISPINX	PC1

	#define RSDIRPORT		PORTD
	#define RSDIRDDR		DDRD
	#define RSDIRPINX		PD6

	#define LEDPORT		PORTC
	#define LEDDDR		DDRC
	#define LEDPINX		PC2

	#define BUTTONPORT	PORTB
	#define BUTTONPIN	PINB
	#define BUTTONDDR	DDRB
	#define BUTTONPINX	PB2

	#define SHDPORT		PORTD
	#define SHDDDR		DDRD
	#define SHDPINX		PD5

#endif

#if (HWVERSION == 1)
	#define RELAISPORT	PORTC
	#define RELAISDDR		DDRC
	#define RELAISPINX	PC1

	#define RSDIRPORT		PORTD
	#define RSDIRDDR		DDRD
	#define RSDIRPINX		PD6

	#define LEDPORT		PORTC
	#define LEDDDR		DDRC
	#define LEDPINX		PC2

	#define BUTTONPORT	PORTC
	#define BUTTONPIN	PINC
	#define BUTTONDDR	DDRC
	#define BUTTONPINX	PC4

	#define SHDPORT		PORTD
	#define SHDDDR		DDRD
	#define SHDPINX		PD5

#endif

#define RELAISON RELAISPORT |= 1<<RELAISPINX
#define RELAISOFF RELAISPORT &= ~(1<<RELAISPINX);
#define RELAISTOGGLE RELAISPORT ^= 1<<RELAISPINX

#endif

