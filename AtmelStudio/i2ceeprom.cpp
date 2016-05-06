/*-----------------------------------------------------------------------------
FILENAME: i2ceeprom.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Implementation of I2CEeprom class
-----------------------------------------------------------------------------*/

#include "global.h"
#include "i2ceeprom.h"
#include <util/delay.h>

I2CEeprom i2ceeprom;

I2CEeprom::I2CEeprom(void)
{
	TWBR = 5;
}

void I2CEeprom::Start(void)
{
	TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN;
	loop_until_bit_is_set(TWCR, TWINT);
}

void I2CEeprom::Stop(void)
{
	TWCR = 1<<TWINT | 1<<TWEN | 1<<TWSTO;
	loop_until_bit_is_clear(TWCR, TWSTO);
}

bool I2CEeprom::SetAddress(uint16_t adr)
{
	while (1)
	{
		uint8_t i = 0;
		
		Start();
		
		TWDR = 0xA0 + I2CEEPROM_ADR; //SLA+W
		TWCR = 1<<TWINT | 1<<TWEN;
		loop_until_bit_is_set(TWCR, TWINT);
		
		if ((TWSR & 0xF8) == 0x18)
			break;
		else
		{
			Stop();
			if (i > 5)
				return false;
			i++;
		}
	}
	
	TWDR = adr >> 8;
	TWCR = 1<<TWINT | 1<<TWEN;
	loop_until_bit_is_set(TWCR, TWINT);
	
	TWDR = adr & 0xFF;
	TWCR = 1<<TWINT | 1<<TWEN;
	loop_until_bit_is_set(TWCR, TWINT);
	return true;
}

bool I2CEeprom::Test(void)
{
	volatile uint16_t d;
	
	TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN;
	
	for (d=0; d<50000; d++);
	if (bit_is_clear(TWCR, TWINT))
	{
		TWCR = 0;
		return false;
	}
	
	TWCR = 0;
	
	bool res = SetAddress(I2CEEPROM_ADR);
	
	if (res)
	{
		Stop();
		return true;
	}
	
	return false;
}

void I2CEeprom::ReadBuf(uint8_t *data, uint16_t adr, uint16_t len)
{
	SetAddress(adr);
	
	TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN;
	loop_until_bit_is_set(TWCR, TWINT);
	
	TWDR = 0xA1 + I2CEEPROM_ADR; //SLA+R
	TWCR = 1<<TWINT | 1<<TWEN;
	loop_until_bit_is_set(TWCR, TWINT);
	
	while (len--)
	{
		if (len)
			TWCR = 1<<TWINT | 1<<TWEN | 1<<TWEA;
		else
			TWCR = 1<<TWINT | 1<<TWEN;
			
		loop_until_bit_is_set(TWCR, TWINT);
		*data++ = TWDR;
	}
	Stop();
}

void I2CEeprom::WriteBuf(const uint8_t *data, uint16_t adr, uint16_t len)
{
	uint16_t currentpage = adr / I2CEEPROM_PAGESIZE;
	
	SetAddress(adr);
	
	while (len--)
	{
		if ((adr / I2CEEPROM_PAGESIZE) > currentpage)
		{
			Stop();
			_delay_ms(10);
			SetAddress(adr);
			currentpage = adr / I2CEEPROM_PAGESIZE;
		}
		
		TWDR = *data++;
		TWCR = 1<<TWINT | 1<<TWEN;
		loop_until_bit_is_set(TWCR, TWINT);
		adr++;
	}
	Stop();
}

void I2CEeprom::Empty(void)
{
	uint16_t i, j;
	uint16_t adr = 0;
	for (i=0; i<I2CEEPROM_SIZE / 8 * 1024 / I2CEEPROM_PAGESIZE; i++)
	{
		SetAddress(adr);
		for (j=0; j<I2CEEPROM_PAGESIZE; j++)
		{
			TWDR = 0xFF;
			TWCR = 1<<TWINT | 1<<TWEN;
			loop_until_bit_is_set(TWCR, TWINT);
			adr++;
		}
		Stop();
	}
}
