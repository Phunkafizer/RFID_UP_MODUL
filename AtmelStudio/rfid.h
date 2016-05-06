/*-----------------------------------------------------------------------------
FILENAME: rfid.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Declaration of rfid class
-----------------------------------------------------------------------------*/

#ifndef rfid_h
#define rfid_h

#include "timer.h"
#include "task.h"

class Rfid: public Task
{
	private:
		uint8_t rawtag[8];
		uint8_t tag[5];
		uint8_t lasttag[5];
		uint8_t bytecnt;
		Timer timer;
		uint8_t rawtag_flag;
		uint8_t tag_flag;
	protected:
	public:
		void InjectByte(uint8_t b);
		void Init(void);
		void TagStart(void);
		void Execute(void);
		void GetTag(uint8_t *tag);
};

extern Rfid rfid;

#endif
