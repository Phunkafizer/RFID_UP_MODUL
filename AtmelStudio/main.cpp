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

#include "timedtask.h"
#include "led.h"

FUSES =
{
	(FUSE_BODEN & FUSE_CKSEL1), //.low
	(FUSE_SPIEN & FUSE_EESAVE & FUSE_BOOTSZ1 & FUSE_BOOTSZ0) // .high
};

class TaskA: public TimedTask {
	bool state;
protected:
	void onTask() {
		if (state) {
			LEDRED;
		}
		else {
			LEDGREEN;
		}
			
		state = !state;
	};
public:
	TaskA(): TimedTask(300) {};
};
TaskA taska;

class TaskB: public TimedTask {
	bool state;
protected:
	void onTask() {
		if (state)
			PORTC |= 1<<PC0;
		else
			PORTC &= ~(1<<PC0);
		state = !state;
	}
public:
	TaskB(): TimedTask(2000) {};
};
//TaskB taskb;




int main(void)
{
	RELAISDDR |= 1<<RELAISPINX;
	
	#ifdef LEDADDR
	LEDADDR |= 1<<LEDAPINX;
	LEDBDDR |= 1<<LEDBPINX;
	#endif
	
	#ifdef LEDDDR
	LEDDDR |= 1<<LEDPINX;
	#endif
	
	PORTB |= 1<<PB3; //enable pull up PB3 (MOSI) for jumper
	
	RSDIRPORT |= 1<<RSDIRPINX;
	DDRD |= 1<<PD4 | 1<<PD3;
	
	sei();
	
	tagmanager.Init();
	rfid.Init();
	
	while (1)
	{
		Task::run();
	}
}
