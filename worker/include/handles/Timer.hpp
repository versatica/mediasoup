#ifndef MS_TIMER_HPP
#define MS_TIMER_HPP

#include "common.hpp"
#include <uv.h>

class Timer
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnTimer(Timer* timer) = 0;
	};

public:
	explicit Timer(Listener* listener);
	Timer& operator=(const Timer&) = delete;
	Timer(const Timer&)            = delete;

private:
	~Timer() = default;

public:
	void Destroy();
	void Start(uint64_t timeout, uint64_t repeat = 0);
	void Stop();
	void Reset();
	bool IsActive() const;

	/* Callbacks fired by UV events. */
public:
	void OnUvTimer();

private:
	// Passed by argument.
	Listener* listener{ nullptr };
	// Allocated by this.
	uv_timer_t* uvHandle{ nullptr };
};

/* Inline methods. */

inline bool Timer::IsActive() const
{
	return uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0;
}

#endif
