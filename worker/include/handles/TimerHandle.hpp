#ifndef MS_TIMER_HANDLE_HPP
#define MS_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include <uv.h>

// Forward declaration.
class Shared;
class BackoffTimerHandle;

class TimerHandle : public TimerHandleInterface
{
	// Only Shared and BackoffTimerHandle classes can invoke the constructor.
	friend class Shared;
	friend class BackoffTimerHandle;

private:
	explicit TimerHandle(TimerHandleInterface::Listener* listener, std::string label);

public:
	TimerHandle& operator=(const TimerHandle&) = delete;

	TimerHandle(const TimerHandle&) = delete;

	~TimerHandle() override;

public:
	void Start(int64_t timeoutMs, int64_t repeatMs = 0) override;

	void Stop() override;

	void Restart() override;

	void Restart(int64_t timeoutMs, int64_t repeatMs = 0) override;

	int64_t GetTimeoutMs() const override
	{
		return this->timeoutMs;
	}

	int64_t GetRepeatMs() const override
	{
		return this->repeatMs;
	}

	bool IsActive() const override
	{
		return uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0;
	}

	const std::string GetLabel() const override
	{
		return this->label;
	}

private:
	void InternalClose();

	/* Callbacks fired by UV events. */
public:
	void OnUvTimer();

private:
	// Passed by argument.
	TimerHandleInterface::Listener* listener{ nullptr };
	const std::string label;
	// Allocated by this.
	uv_timer_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	int64_t timeoutMs{ 0 };
	int64_t repeatMs{ 0 };
};

#endif
