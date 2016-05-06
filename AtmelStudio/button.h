/*-----------------------------------------------------------------------------
FILENAME: button.h
AUTHOR: S. Seegel, post@seegel-systeme.de
DATE: 17.8.2008
VERSION: 1.0
LICENSE: GPL v3
DESCRIPTION: Declaration of Button class
-----------------------------------------------------------------------------*/

#ifndef button_h
#define button_h

#include <avr/io.h>
#include "timer.h"
#include "task.h"

class Button: public Task
{
	private:
		Timer timer;
		bool progmode;
		uint8_t laststate;
	protected:
		void Execute(void);
	public:
		Button(void);
		bool IsProgMode(void) {return progmode;};
		void ResetProgMode(void);
};

extern Button button;

#endif
