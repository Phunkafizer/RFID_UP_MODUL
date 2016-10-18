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
#include "config.h"
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
		maxnumtags = I2CEEPROM_SIZE * 128UL / sizeof(tag_item_t);
	else
		maxnumtags = ((E2END + 1 - INT_EEPROM_RESERVED) / sizeof(tag_item_t));
	
	//OCR1A = F_CPU / SEC_TIMER_DIV - 1;
	//TIMSK |= 1<<OCIE1A;
	//TCCR1B = 1<<CS10;
}

void Tagmanager::Init(void)
{
	numtags = 0;
	
	tag_item_t tag;
	
	while (ReadTag(numtags, &tag))
		numtags++;
}

void Tagmanager::Execute(void)
{
	tag_item_t temp;
	
	switch (mode)
	{
		case TMM_LOOKUP:	
			if (ReadTag(eepromIdx, &temp)) {
				if (memcmp(&temp, &tag, sizeof(tag_item_t)) == 0) {
					//Tag found on local mem
					if ((eepromIdx == 0) && (eeprom_read_byte(&eeconfig.modulemode) == MODE_ADMINTAG))
						EnterAdminTagMode();
					else {
						mode = TMM_IDLE;
						relay.Trigger();
						admintag = false;
						button.ResetProgMode();
						bus.SendTagRequest(&tag, true);
					}
				}
				else
					eepromIdx++;
			}
			else {
				//tag was not found in eeprom
				mode = TMM_IDLE;
				led.SetState(STATE_IDLE);
				uint8_t accessed = false;
				uint8_t modulemode = eeprom_read_byte(&eeconfig.modulemode);
				
				if ((eepromIdx == 0) && (modulemode != MODE_BUS) && (button.IsProgMode() == false)) {
					//when no tag is in memory, access (e.g. for checking a new module)
					accessed = true;
				}
				else
					switch (modulemode) {
						case MODE_BUTTONADD:
							if (button.IsProgMode())
							{
								WriteTag(&tag, eepromIdx);
								accessed = true;
							}
							break;
					
						case MODE_ADMINTAG:
							if (button.IsProgMode())
								WriteTag(&tag, 0);
							else
							{
								if (admintag)
								{
									WriteTag(&tag, eepromIdx);
									accessed = true;
								}
							}
							break;
					}
				
				admintag = false;
				button.ResetProgMode();
				
				if (accessed)
					relay.Trigger(); //this also sets the led
				else
					led.SetState(STATE_DENY);
				
				bus.SendTagRequest(&tag, accessed);
			}
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

void Tagmanager::ProcessTag(tag_item_t *tag)
{
	if (mode == TMM_IDLE)
	{
		eepromIdx = 0;
		memcpy(&this->tag, tag, sizeof(tag_item_t));
		mode = TMM_LOOKUP;
	}
}

bool Tagmanager::ReadTag(uint16_t idx, tag_item_t *tag)
{
	if (idx < maxnumtags) {
		uint16_t adr = idx * sizeof(tag_item_t);
		if (useexteeprom)
			i2ceeprom.ReadBuf((uint8_t *) tag, adr, sizeof(tag_item_t));
		else
			eeprom_read_block(tag, (uint8_t *) adr + INT_EEPROM_RESERVED, sizeof(tag_item_t));
	}
	else {
		tag->tagtype = TAG_NONE;
	}
	
	return (tag->tagtype != TAG_NONE);
}

uint16_t Tagmanager::AddTag(tag_item_t *tag)
{	
	uint16_t idx;
	tag_item_t temp;
	
	for (idx=0; idx<maxnumtags; idx++)
	{
		if (ReadTag(idx, &temp)) {
			if (memcmp(&temp, tag, sizeof(tag_item_t)) == 0) //tag already exists
				return 0xFFFF;
		}
		else
			break;
	}
	
	if (idx < maxnumtags - 1) {
		WriteTag(tag, idx);
		return idx;
	}
	
	return 0xFFFF;
}

uint16_t Tagmanager::WriteTag(tag_item_t *tag, uint16_t idx)
{
	uint16_t adr = idx * sizeof(tag_item_t);
	
	if (idx < maxnumtags)
	{
		if (useexteeprom)
			i2ceeprom.WriteBuf((uint8_t *) tag, adr, sizeof(tag_item_t));
		else
			eeprom_write_block(tag, (uint8_t *) adr + INT_EEPROM_RESERVED, sizeof(tag_item_t));
		return idx;
	}
	
	return 0xFFFF;
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
