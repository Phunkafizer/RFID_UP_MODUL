/*
	task.h
	Stefan Seegel, post@seegel-systeme.de
	feb 2011
	last update apr 2016
	http://opensource.org/licenses/gpl-license.php GNU Public License
	Implementation of Task class
*/

#include "task.h"

Task* Task::last = 0;

void Task::run() {
	static Task* item = 0;
	
	if (item) {
		item->Execute();
		item = item->prior;
	}
	else
		item = last;
}

Task::Task(void)
{
	//append to list
	prior = last;
	last = this;
}
