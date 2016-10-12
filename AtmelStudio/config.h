#ifndef config_h
#define config_h

#define HWADDRESS 0x0005
#define MODE MODE_BUTTONADD

#include <avr/eeprom.h>

typedef struct 
{
	uint16_t hostadr;			//Adress of device. 0xFFFF is unadressed mode
	uint8_t modulemode;			//0xFF = Admintag, 0x00 = Buttonmode, 0x01 = Busmode
	uint8_t triggertime;		//triggertime of relay 1/10 s, 0 = toggle
	uint8_t relay_flags;		
} eeconfig_t;

#define MODE_ADMINTAG	0xFF
#define MODE_BUTTONADD	0x00
#define MODE_BUS		0x01

#define RELAY_FLAG_POLARITY			0	//1: normal, 0: inverted
#define RELAY_FLAG_RETRIGGERABLE	1	//1: not retriggerable, o: retriggerable

#define INT_EEPROM_RESERVED	16 //Number of bytes reserved at the end of int eeprom

extern eeconfig_t eeconfig EEMEM;
#endif
