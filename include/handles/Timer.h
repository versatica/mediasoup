#ifndef MS_TIMER_H
#define	MS_TIMER_H

#include "common.h"
#include <uv.h>

class Timer
{
public:
	class Listener
	{
	public:
		virtual void onTimer(Timer* timer) = 0;
	};

public:
	Timer(Listener* listener);

	void Start(MS_8BYTES timeout);
	void Stop();
	void Close();

/* Callbacks fired by UV events. */
public:
	void onUvTimer();

private:
	// Allocated by this:
	uv_timer_t* uvHandle = nullptr;
	// Passed by argument:
	Listener* listener = nullptr;
};

#endif
