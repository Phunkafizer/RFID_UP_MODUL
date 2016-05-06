/*-----------------------------------------------------------------------------
FILENAME: button.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Implementation of Button class
-----------------------------------------------------------------------------*/

#include <avr/eeprom.h>
#include <string.h>
#include "button.h"
#include "global.h"
#include "led.h"
#include "eeprom.h"
#include "i2ceeprom.h"

#include "bus.h"

Button button;

Button::Button(void)
{
	BUTTONPORT |= 1<<BUTTONPINX; //enable pull up for button input
	laststate = 1<<BUTTONPINX;
	progmode = false;
}

void Button::Execute(void)
{
	uint8_t state;
	
	state = BUTTONPIN & (1<<BUTTONPINX);
	
	if (state != laststate)
	{
		laststate = state;
		if (!state)
			timer.SetTime(2000);
		else
		{
			progmode = true;
			bus.SendIOEvent(0);
			
			switch (eeprom_read_byte(&eeconfig.modulemode))
			{
				case MODE_ADMINTAG:
					//#ifdef USEEXTEEPROM
					//i2ceeprom.ReadBuf(tag, 0, 5);
					//#else
					//eeprom_read_block(tag, (uint8_t *) 0, 5);
					//#endif
					//if (memcmp(tag, empty_tag, 5) == 0)
						led.SetState(STATE_SETADMINTAG);
					//else
						//progmode = false;
					break;
					
				case MODE_BUTTONADD:
					led.SetState(STATE_ADDTAG);
					break;
			}
			
			timer.SetTime(10000);
		}
	}
	
	if (timer.IsFlagged())
	{
		if (progmode)
		{
			progmode = false;
			led.SetState(STATE_IDLE);
		}
		else
		{
			led.SetState(STATE_DELETING);
			uint16_t i;
			if (useexteeprom)
				i2ceeprom.Empty();
			else
				for (i=INT_EEPROM_RESERVED; i<E2END; i++)
					eeprom_write_byte((uint8_t *) i, 0xFF);
			led.SetState(STATE_IDLE);
		}
		
		loop_until_bit_is_set(BUTTONPIN, BUTTONPINX);
		laststate = state = 1<<BUTTONPINX;
	}
}

void Button::ResetProgMode(void)
{
	progmode = false;
	timer.SetTime(0);
}
