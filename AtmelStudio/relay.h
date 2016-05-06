#ifndef relay_h
#define relay_h

#include "timer.h"
#include "task.h"


class Relay: public Task
{
	private:
		Timer timer;
		uint16_t triggertime;
		bool triggered;
		bool timetriggered;
		void SetRelay(bool active);
	protected:
		void Execute(void);
	public:
		Relay(void);
		void Trigger(void);
		void TriggerTime(uint16_t time);
};

extern Relay relay;

#endif