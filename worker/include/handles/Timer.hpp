#ifndef MS_TIMER_HPP
#define	MS_TIMER_HPP

#include "common.hpp"
#include <uv.h>

class Timer
{
public:
	class Listener
	{
	public:
		virtual ~Listener() {};

	public:
		virtual void onTimer(Timer* timer) = 0;
	};

public:
	explicit Timer(Listener* listener);
	Timer& operator=(const Timer&) = delete;
	Timer(const Timer&) = delete;

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
