#include "relay.h"
#include "led.h"
#include "global.h"
#include "config.h"

Relay relay;

Relay::Relay(void)
{
	SetRelay(false);
	triggered = false;
	timetriggered = false;
}

void Relay::Trigger(void)
{
	if (timetriggered)
		return;
		
	uint8_t temp, relayflags;
	temp = eeprom_read_byte(&eeconfig.triggertime);
	relayflags = eeprom_read_byte(&eeconfig.relay_flags);
	
	if (temp == 0xFF)
		triggertime = 1000;
	else
		triggertime = temp * 100UL;
	
	led.SetState(STATE_ACCESS);
	
	if (triggertime == 0)
	{
		RELAISTOGGLE;
		timer.SetTime(1000);
	}
	else
	{
		if ((!triggered) || (relayflags & 1<<RELAY_FLAG_NOT_RETRIGGERABLE) == 0)
		{
			triggered = true;
			timer.SetTime(triggertime);
			SetRelay(true);
		}
	}
}


void Relay::TriggerTime(uint16_t time)
{
	led.SetState(STATE_ACCESS);
	
	if (time)
	{
		timer.SetTime(time);
		timetriggered = true;
		triggertime = 1;
		SetRelay(true);
	}
	else
		SetRelay(false);
}

void Relay::SetRelay(bool active)
{
	if (((eeprom_read_byte(&eeconfig.relay_flags) & 1<<RELAY_FLAG_POLARITY)) ^ active)
	{
		RELAISOFF;
	}
	else
	{
		RELAISON;
	}
}

void Relay::Execute(void)
{
	if (timer.IsFlagged())
	{
		triggered = false;
		timetriggered = false;
		if (triggertime)
		{
			SetRelay(false);
		}
				
		led.SetState(STATE_IDLE);
	}
}