/*-----------------------------------------------------------------------------
FILENAME: main.cpp
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 22.7.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION:  Some hardware initialization and main loop
-----------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/fuse.h>
#include "task.h"
#include "rfid.h"
#include "tagmanager.h"
#include "global.h"

FUSES =
{
	(FUSE_BODEN & FUSE_CKSEL1), //.low
	(FUSE_SPIEN & FUSE_EESAVE & FUSE_BOOTSZ1 & FUSE_BOOTSZ0) // .high
};

int main(void)
{
	Rfid rfid;

	RELAISDDR |= 1<<RELAISPINX;
	RSDIRDDR |= 1<<RSDIRPINX;
	
	#ifdef LEDADDR
	LEDADDR |= 1<<LEDAPINX;
	LEDBDDR |= 1<<LEDBPINX;
	#endif
	
	#ifdef LEDDDR
	LEDDDR |= 1<<LEDPINX;
	#endif
	
	PORTB |= 1<<PB3; //enable pull up PB3 (MOSI) for jumper
	
	sei();
	
	tagmanager.Init();
	rfid.Init();
	
	while (1)
	{
		Task::run();

		tag_item_t tag;
		tag.tagtype = rfid.getTag(tag.data);
		if (tag.tagtype != TAG_NONE)
			tagmanager.ProcessTag(&tag);
	}
}
