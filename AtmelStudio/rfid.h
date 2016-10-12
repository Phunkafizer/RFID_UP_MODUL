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

#define RFIDCNTREG TCNT2
typedef uint8_t rfidcnt_t;

enum tag_t {TAG_NONE = 0, TAG_EM4100 = 1, TAG_FDXB = 2};

#define STR(x) #x
#define HLPSTR(x) STR(x)

class Rfid: public Task {
private:
	Timer timer;
	uint8_t tagdata[6];
	enum tag_t currentTag;
	bool decodeEm4100();
	bool decodeFDX();
	static bool em_flag;
	static bool fdx_flag;
	static void irq() __asm__(HLPSTR(INT0_vect)) __attribute__((__signal__, __used__, __externally_visible__));
protected:
	volatile static uint8_t buffer[16];
public:
	void Init(void);
	void TagStart(void);
	void Execute(void);
	enum tag_t getTag(uint8_t *tag);
};

#undef STR
#undef HLPSTR

#endif
