#ifndef MS_TIMER_HANDLE_INTERFACE_HPP
#define MS_TIMER_HANDLE_INTERFACE_HPP

#include "common.hpp"
#include <string>

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
	virtual void Start(int64_t timeoutMs, int64_t repeatMs = 0) = 0;

	virtual void Stop() = 0;

	virtual void Restart() = 0;

	virtual void Restart(int64_t timeoutMs, int64_t repeatMs = 0) = 0;

	virtual int64_t GetTimeoutMs() const = 0;

	virtual int64_t GetRepeatMs() const = 0;

	virtual bool IsActive() const = 0;

	/**
	 * Label of this timer, given at creation time.
	 */
	virtual const std::string GetLabel() const = 0;
};

#endif
