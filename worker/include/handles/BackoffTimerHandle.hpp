#ifndef MS_BACKOFF_TIMER_HANDLE_HPP
#define MS_BACKOFF_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "handles/TimerHandle.hpp"
#include "handles/TimerHandleInterface.hpp"

// Forward declaration.
class Shared;
// TODO: Temporal until we have MockBackoffTimerHandle.
namespace mocks
{
	class MockShared;
}

class BackoffTimerHandle : public BackoffTimerHandleInterface, public TimerHandleInterface::Listener
{
	// Only Shared class can invoke the constructor.
	friend class Shared;
	// TODO: Temporal until we have MockBackoffTimerHandle.
	friend class mocks::MockShared;

private:
	explicit BackoffTimerHandle(const BackoffTimerHandleOptions& options);

public:
	BackoffTimerHandle& operator=(const BackoffTimerHandle&) = delete;

	BackoffTimerHandle(const BackoffTimerHandle&) = delete;

	~BackoffTimerHandle() override;

public:
	/**
	 * Start the BackoffTimer (if it's stopped) or restart it (if already
	 * running). It will reset the timeout count.
	 */
	void Start() override;

	/**
	 * Stop the BackoffTimer. It will reset the timeout count.
	 */
	void Stop() override;

	/**
	 * Set the base timeout duration. It will be applied after the next timeout
	 * and effective duration can be larger if backoff algorithm is exponential.
	 */
	void SetBaseTimeoutMs(uint64_t baseTimeoutMs) override;

	/**
	 * Whether the BackoffTimer is running. Useful to check if this BackoffTimer
	 * will timeout again within the OnTimer() callback.
	 */
	bool IsRunning() const override
	{
		return this->running;
	}

	/**
	 * Maximum number of restarts.
	 *
	 * @remarks
	 * - If `maxRestarts` was not given in the constructor, this method returns
	 *   `std::nullopt`.
	 */
	std::optional<size_t> GetMaxRestarts() const override
	{
		return this->maxRestarts;
	}

	/**
	 * Number of times the timer has expired.
	 */
	size_t GetExpirationCount() const override
	{
		return this->expirationCount;
	}

private:
	uint64_t ComputeNextTimeoutMs() const;

	/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
public:
	void OnTimer(TimerHandleInterface* timer) override;

private:
	// Passed by argument.
	BackoffTimerHandleInterface::Listener* listener{ nullptr };
	uint64_t baseTimeoutMs{ 0 };
	BackoffAlgorithm backoffAlgorithm;
	std::optional<uint64_t> maxBackoffTimeoutMs;
	std::optional<size_t> maxRestarts;
	// Allocated by this.
	TimerHandle* timer{ nullptr };
	// Others.
	bool running{ false };
	size_t expirationCount{ 0 };
};

#endif
