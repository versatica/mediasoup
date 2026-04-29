#ifndef MS_TIMER_HANDLE_HPP
#define MS_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include <uv.h>

// Forward declaration.
class Shared;
class BackoffTimerHandle;
// TODO: Temporal until we have MockBackoffTimerHandle.
namespace mocks
{
	class MockShared;
}

class TimerHandle : public TimerHandleInterface
{
	// Only Shared and BackoffTimerHandle classes can invoke the constructor.
	friend class Shared;
	friend class BackoffTimerHandle;
	// TODO: Temporal until we have MockBackoffTimerHandle.
	friend class mocks::MockShared;

private:
	explicit TimerHandle(TimerHandleInterface::Listener* listener);

public:
	TimerHandle& operator=(const TimerHandle&) = delete;

	TimerHandle(const TimerHandle&) = delete;

	~TimerHandle() override;

public:
	void Start(uint64_t timeout, uint64_t repeat = 0) override;

	void Stop() override;

	void Restart() override;

	void Restart(uint64_t timeout, uint64_t repeat = 0) override;

	uint64_t GetTimeout() const override
	{
		return this->timeout;
	}

	uint64_t GetRepeat() const override
	{
		return this->repeat;
	}

	bool IsActive() const override
	{
		return uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0;
	}

private:
	void InternalClose();

	/* Callbacks fired by UV events. */
public:
	void OnUvTimer();

private:
	// Passed by argument.
	TimerHandleInterface::Listener* listener{ nullptr };
	// Allocated by this.
	uv_timer_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	uint64_t timeout{ 0u };
	uint64_t repeat{ 0u };
};

#endif
