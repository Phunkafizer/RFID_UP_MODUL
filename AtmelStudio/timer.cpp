/*
	timer.cpp
	timer class
	Stefan Seegel, post@seegel-systeme.de
	created feb 2011
	last update apr 2016
	http://opensource.org/licenses/gpl-license.php GNU Public License
*/
#include "timer.h"
#include <avr/interrupt.h>

volatile static bool temp_idle = true;
volatile bool timers_idle = true;

Timer* Timer::lastobj = 0;

Timer::Timer(bool oneshot) {
	this->oneshot = oneshot;
	Init();
}

Timer::Timer(uint16_t time, bool oneshot) {
	this->oneshot = oneshot;
	Init();
	SetTime(time);
}

void Timer::Init() {
	flag = false;
	cnt = 0;
	
	if (lastobj == 0)
	{//this is the first timer object, init hardware timer
		PRESCALEREG = CSVAL;
		OCRREG += OCRVAL;
		TIMSK |= 1<<IRQFLAGBIT;
	}
	
	//append to list
	prior = lastobj;
	lastobj = this;
}


bool Timer::IsFlagged(void) {
	if (flag) {	
		flag = false;
		if (!oneshot)
			SetTime(loadval);
		return true;
	}
	
	return false;
}

void Timer::SetTime(uint16_t ticks) {
	TIMSK &= ~(1<<IRQFLAGBIT);
	cnt = ticks;
	if (ticks)
		timers_idle = false;
	
	flag = 0;
	loadval = ticks;
		
	TIMSK |= 1<<IRQFLAGBIT;
}

void Timer::irq() {
	static int32_t remainder = 0;
	remainder += F_CPU % TOTALDIV;
	if (remainder > (int32_t) (TOTALDIV / 2)) {
		remainder -= TOTALDIV;
		OCRREG += OCRVAL + 1;
	}
	else
		OCRREG += OCRVAL;
		
	temp_idle = true;
	
	Timer *item = lastobj;
	while (item)
	{
		if (item->cnt)
		{
			temp_idle = false;
			if (--item->cnt == 0)
				item->flag = true;
		}
		item = item->prior;
	}
	timers_idle = temp_idle;
}