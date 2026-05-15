#ifndef MS_TIMER_HANDLE_INTERFACE_HPP
#define MS_TIMER_HANDLE_INTERFACE_HPP

#include "common.hpp"

class TimerHandleInterface
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnTimer(TimerHandleInterface* timer) = 0;
	};

public:
	TimerHandleInterface() = default;

	TimerHandleInterface& operator=(const TimerHandleInterface&) = delete;

	TimerHandleInterface(const TimerHandleInterface&) = delete;

	virtual ~TimerHandleInterface() = default;

public:
	virtual void Start(uint64_t timeout, uint64_t repeat = 0) = 0;

	virtual void Stop() = 0;

	virtual void Restart() = 0;

	virtual void Restart(uint64_t timeout, uint64_t repeat = 0) = 0;

	virtual uint64_t GetTimeout() const = 0;

	virtual uint64_t GetRepeat() const = 0;

	virtual bool IsActive() const = 0;
};

#endif
