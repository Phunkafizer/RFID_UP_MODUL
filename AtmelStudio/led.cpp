#include "led.h"
#include "global.h"

Led led;

Led::Led(void)
{
	timer.SetTime(200);
	state = STATE_IDLE;
}

void Led::Execute(void)
{
	return;
	if (timer.IsFlagged())
	{
		timer.SetTime(200);
		
		switch (state)
		{	
			case STATE_IDLE:
				LEDOFF;
				break;
				
			case STATE_SETADMINTAG:
				blinkstate = !blinkstate;
				timer.SetTime(500);
				if (blinkstate)
				{
					#ifdef LEDRED
					LEDRED;
					#endif
					#ifdef LEDON
					LEDON;
					#endif
				}
				else
					LEDOFF;
				break;
				
			case STATE_ADDTAG:
				blinkstate = !blinkstate;
				timer.SetTime(100);
				if (blinkstate)
				{
					#ifdef LEDGREEN
					LEDGREEN;
					#endif
					#ifdef LEDON
					LEDON;
					#endif
				}
				else
					LEDOFF;
				break;
				
			case STATE_ACCESS:
				#ifdef LEDGREEN
				LEDGREEN;
				#endif
				#ifdef LEDON
				LEDON;
				#endif
				break;
				
			case STATE_DENY:
				#ifdef LEDRED
				LEDRED;
				#endif
				#ifdef LEDON
				LEDON;
				#endif
				
				timer.SetTime(1000);
				state = STATE_IDLE;
				break;
		}
	}
}

void Led::SetState(uint8_t state)
{
	this->state = state;
	if (state == STATE_DELETING)
	{
		#ifdef LEDRED
			LEDRED;
		#endif
		#ifdef LEDON
			LEDON;
		#endif
	}
	timer.SetTime(2); //Force Execute
}