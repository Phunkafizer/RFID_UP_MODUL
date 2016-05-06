/*
 * timedtask.cpp
 *
 * Created: 04.05.2016 15:54:52
 *  Author: sseegel
 */ 

#include "timedtask.h"

TimedTask::TimedTask(uint16_t ticks): timer(ticks, false) {
}

void TimedTask::Execute() {
	if (timer.IsFlagged())
		onTask();
}
