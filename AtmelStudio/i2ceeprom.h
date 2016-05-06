/*-----------------------------------------------------------------------------
FILENAME: i2ceeprom.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Declaration of I2CEeprom class
-----------------------------------------------------------------------------*/

#ifndef i2ceeprom_h
#define i2ceeprom_h

#include <avr/io.h>

#define I2CEEPROM_ADR	0	//lower address of connected I2C eeprom (0-7)
#define I2CEEPROM_SIZE	64UL	//Size of I2C eeprom in kbits
#define I2CEEPROM_PAGESIZE	32

class I2CEeprom
{
	private:
		inline void Start(void);
		inline void Stop(void);
		bool SetAddress(uint16_t adr);
	protected:
	public:
		I2CEeprom(void);
		bool Test(void);
		void ReadBuf(uint8_t *data, uint16_t adr, uint16_t len);
		void WriteBuf(const uint8_t *data, uint16_t adr, uint16_t len);
		void Empty(void);
};

extern I2CEeprom i2ceeprom;

#endif
