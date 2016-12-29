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
	explicit Timer(Listener* listener);

	void Close();
	void Start(uint64_t timeout);
	void Stop();

/* Callbacks fired by UV events. */
public:
	void onUvTimer();

private:
	// Passed by argument.
	Listener* listener = nullptr;
	// Allocated by this.
	uv_timer_t* uvHandle = nullptr;
};

#endif
