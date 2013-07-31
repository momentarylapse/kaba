/*
 * HuiTimer.h
 *
 *  Created on: 25.06.2013
 *      Author: michi
 */

#ifndef HUITIMER_H_
#define HUITIMER_H_

#include "hui.h"

void HuiInitTimers();

class HuiTimer
{
public:
	HuiTimer();
	float peek();
	float get();
	void reset();

private:
#ifdef OS_WINDOWS
	LONGLONG cur_time;
	LONGLONG last_time;
#endif
#ifdef OS_LINUX
	struct timeval cur_time;
	struct timeval last_time;
#endif
};


void _cdecl HuiSleep(float duration);

#endif /* HUITIMER_H_ */