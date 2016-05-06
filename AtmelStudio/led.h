#ifndef _led_h
#define _led_h

#include "timer.h"
#include "task.h"

#define STATE_IDLE			0
#define STATE_SETADMINTAG	1
#define STATE_ADDTAG		2
#define STATE_ACCESS		3
#define STATE_DENY			4
#define STATE_DELETING		5

#if (HWVERSION == 3)
	//#define PERMANENT_ILLUM
	#ifdef PERMANENT_ILLUM
	#define LEDRED		{LEDAPORT &= ~(1<<LEDAPINX); LEDBPORT &= ~(1<<LEDBPINX);}
	#define LEDGREEN	{LEDAPORT |= 1<<LEDAPINX; LEDBPORT &= ~(1<<LEDBPINX);}
	#define LEDOFF		{LEDAPORT &= ~(1<<LEDAPINX); LEDBPORT |= 1<<LEDBPINX;}
	#else
	#define LEDOFF		{LEDAPORT &= ~(1<<LEDAPINX); LEDBPORT &= ~(1<<LEDBPINX);}
	#define LEDRED		{LEDAPORT |= 1<<LEDAPINX; LEDBPORT &= ~(1<<LEDBPINX);}
	#define LEDGREEN	{LEDAPORT &= ~(1<<LEDAPINX); LEDBPORT |= 1<<LEDBPINX;}
	#endif
#endif

#if (HWVERSION == 2)
	#define LEDOFF		LEDPORT &= ~(1<<LEDPINX);
	#define LEDON		LEDPORT |= 1<<LEDPINX;
#endif

#if (HWVERSION == 1)
	#define LEDOFF		LEDPORT &= ~(1<<LEDPINX);
	#define LEDON		LEDPORT |= 1<<LEDPINX;
#endif



class Led: public Task
{
	private:
		Timer timer;
		uint8_t state;
		bool blinkstate;
	protected:
		void Execute(void);
		
	public:
		Led(void);
		void SetState(uint8_t state);
	
};

extern Led led;

#endif
