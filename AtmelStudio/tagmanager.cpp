/*-----------------------------------------------------------------------------
FILENAME: tagmanager.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Implementation of tagmanager class
-----------------------------------------------------------------------------*/

#include "tagmanager.h"
#include <string.h>
#include "global.h"
#include "led.h"
#include <avr/interrupt.h>
#include "bus.h"
#include "eeprom.h"
#include "button.h"
#include "relay.h"

#include "i2ceeprom.h"
#include <avr/eeprom.h>

Tagmanager tagmanager;

#define SEC_TIMER_DIV 256L
//static uint32_t seconds;
static uint8_t prescaler;

Tagmanager::Tagmanager(void)
{
	mode = TMM_IDLE;
	admintag = false;
	
	useexteeprom = i2ceeprom.Test();
	
	if (useexteeprom)
		maxnumtags = I2CEEPROM_SIZE * 128UL / 5;
	else
		maxnumtags = ((E2END + 1 - INT_EEPROM_RESERVED) / 5);
	
	OCR1A = F_CPU / SEC_TIMER_DIV - 1;
	TIMSK |= 1<<OCIE1A;
	TCCR1B = 1<<CS10;
}

void Tagmanager::Init(void)
{
	numtags = 0;
	
	uint8_t tag[5];
	
	while (true)
	{
		ReadTag(numtags, tag);
		if (memcmp(tag, empty_tag, 5) == 0)
			break;
		numtags++;
	}
	
}

void Tagmanager::Execute(void)
{
	uint8_t temp[5];
	
	switch (mode)
	{
		case TMM_LOOKUP:	
			ReadTag(eeprom_ptr / 5, temp);
						
			if (memcmp(temp, tag, 5) == 0)
			{//Tag found on local mem
				if ((eeprom_ptr == 0) && (eeprom_read_byte(&eeconfig.modulemode) == MODE_ADMINTAG))
					EnterAdminTagMode();
				else
				{//Tag found
					mode = TMM_IDLE;
					relay.Trigger();
					admintag = false;
					button.ResetProgMode();
					bus.SendTagRequest(tag, true);
				}
			}
			else
				if (memcmp(temp, empty_tag, 5) == 0)
				{//eeprom tag is empty, we are at the end of eeprom-tags, tag was not found
					mode = TMM_IDLE;
					led.SetState(STATE_IDLE);	
					uint8_t accessed = false;
					uint8_t modulemode = eeprom_read_byte(&eeconfig.modulemode);
					
					
					if ((eeprom_ptr == 0) && (modulemode != MODE_BUS) && (button.IsProgMode() == false))
					{//when no tag is in memory, access (e.g. for checking a new module)
						accessed = true;
					}
					else
						switch (modulemode)
						{
							case MODE_BUTTONADD:
								if (button.IsProgMode())
								{
									WriteTag(tag, eeprom_ptr / 5);
									accessed = true;
								}
								else
								{
									//Deny
								}
								break;
							
							case MODE_ADMINTAG:
								if (button.IsProgMode())
								{
									WriteTag(tag, 0);
								}
								else
								{
									if (admintag)
									{
										WriteTag(tag, eeprom_ptr / 5);
										accessed = true;
									}
									else
										accessed = false;
								}
								break;
						}
						
					admintag = false;
					button.ResetProgMode();
					
					if (accessed)
						relay.Trigger(); //this also sets the led
					else
						led.SetState(STATE_DENY);
					
					bus.SendTagRequest(tag, accessed);
				}
				else
					eeprom_ptr += 5;
			break;

			
		case TMM_IDLE:
			if (timer.IsFlagged())
			{
				admintag = false;
				
				led.SetState(STATE_IDLE);
			}
			
			break;
	}
}

void Tagmanager::EnterAdminTagMode(void)
{
	mode = TMM_IDLE;
	timer.SetTime(5000);
	admintag = true;
	led.SetState(STATE_ADDTAG);
}

void Tagmanager::ProcessTag(uint8_t *tag)
{
	if (mode == TMM_IDLE)
	{
		eeprom_ptr = 0;
		memcpy(this->tag, tag, 5);
		//uart.SendData(tag, 5);
		mode = TMM_LOOKUP;
	}
}

void Tagmanager::ReadTag(uint16_t i, uint8_t *tag)
{
	uint16_t adr = i * 5;
	
	if (i >= MAX_NUM_TAGS)
		memcpy(tag, empty_tag, 5);
	else
		
	if (useexteeprom)
		i2ceeprom.ReadBuf(tag, adr, 5);
	else
		eeprom_read_block(tag, (uint8_t *) adr + INT_EEPROM_RESERVED, 5);
}

uint16_t Tagmanager::AddTag(uint8_t *tag)
{	
	uint16_t i;
	uint8_t temp[5];
	
	for (i=0; i<MAX_NUM_TAGS; i++)
	{
		ReadTag(i, temp);
				
		if (memcmp(temp, tag, 5) == 0)
			return 0xFFFF;
		else
		{
			if (memcmp(temp, empty_tag, 5) == 0)
			{
				WriteTag(tag, i);
				return i;
			}
		}
	}
	
	if (i < MAX_NUM_TAGS)
		return i;
	
	return 0xFFFF;
}

uint8_t Tagmanager::WriteTag(uint8_t *tag, uint16_t id)
{
	uint16_t adr = id * 5;
	
	if (id < MAX_NUM_TAGS)
	{
		if (useexteeprom)
			i2ceeprom.WriteBuf(tag, adr, 5);
		else
			eeprom_write_block(tag, (uint8_t *) adr + INT_EEPROM_RESERVED, 5);
		return 1;
	}
	
	return 0;
}

void Tagmanager::GiveAccess(void)
{	
	timer.SetTime(2000);
}

void Tagmanager::DenyAccess(void)
{
	mode = TMM_IDLE;
	timer.SetTime(500);
	led.SetState(STATE_DENY);
}

void Tagmanager::AddCurrentTag(void)
{
	mode = TMM_IDLE;
	led.SetState(STATE_ACCESS);
	WriteTag(tag, eeprom_ptr / 5);
}

ISR (TIMER1_COMPA_vect)
{
	if (--prescaler == 0)
	{
		prescaler = (uint8_t) SEC_TIMER_DIV;
		//seconds++;
	}
#if F_CPU % SEC_TIMER_DIV
	if (prescaler <= F_CPU % SEC_TIMER_DIV)
		OCR1A += F_CPU / SEC_TIMER_DIV + 1;
	else
#endif
		OCR1A += F_CPU / SEC_TIMER_DIV;
}
