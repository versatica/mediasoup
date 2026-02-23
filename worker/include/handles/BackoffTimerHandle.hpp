#ifndef MS_BACKOFF_TIMER_HANDLE_HPP
#define MS_BACKOFF_TIMER_HANDLE_HPP

#include "common.hpp"
#include "handles/TimerHandle.hpp"
#include <limits> // std::numeric_limits()

class BackoffTimerHandle : public TimerHandle::Listener
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		/**
		 * Invoked on timeout expiration. The parent can modify the base
		 * timeout given as reference and affect the next timeout duration.
		 *
		 * @remarks
		 * - If the caller deletes this instance of SmartTimer within the callback
		 *   it must signal it be setting `stop` to true.
		 */
		virtual void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) = 0;
	};

public:
	enum class BackoffAlgorithm
	{
		// The base duration will be used for any restart.
		FIXED,
		// An exponential backoff is used for restarts, with a 2x multiplier,
		// meaning that every restart will use a duration that is twice as long as
		// the previous.
		EXPONENTIAL,
	};

public:
	static constexpr uint64_t MaxTimeoutMs{ std::numeric_limits<uint64_t>::max() / 2 };

public:
	explicit BackoffTimerHandle(
	  /**
	   * Listener on which OnTimer() callback will be invoked.
	   */
	  Listener* listener,
	  /**
	   * Base timeout duration (ms).
	   */
	  uint64_t baseTimeoutMs,
	  /**
	   * Backoff algorithm.
	   */
	  BackoffAlgorithm backoffAlgorithm,
	  /**
	   * Maximum duration of the backoff timeout (ms). If no value is given, no
	   * limit is set.
	   */
	  std::optional<uint64_t> maxBackoffTimeoutMs,
	  /**
	   * Maximum number of restarts. If no value is given, it will restart
	   * forever until stopped.
	   */
	  std::optional<size_t> maxRestarts);

	BackoffTimerHandle& operator=(const BackoffTimerHandle&) = delete;

	BackoffTimerHandle(const BackoffTimerHandle&) = delete;

	~BackoffTimerHandle();

public:
	/**
	 * Start the smart timer (if it's stopped) or restart it (if already
	 * running). It will reset the timeout count.
	 */
	void Start();

	/**
	 * Stop the smart timer. It will reset the timeout count.
	 */
	void Stop();

	/**
	 * Restart the smart timer (if it's active) or start it. It will reset the
	 * timeout count.
	 */
	void Restart();

	/**
	 * Get the base timeout duration.
	 */
	uint64_t GetBaseTimeoutMs() const
	{
		return this->baseTimeoutMs;
	}

	/**
	 * Set the base timeout duration. It will be applied after the next timeout
	 * and effective duration can be larger if backoff algorithm is exponential.
	 */
	void SetBaseTimeoutMs(uint64_t baseTimeoutMs);

	/**
	 * Whether the smart timer is running. Useful to check if this smart timer
	 * will timeout again within the OnTimer() callback.
	 */
	bool IsActive() const
	{
		return this->active;
	}

	/**
	 * Number of times the timer has expired.
	 */
	size_t GetTimeoutCount() const
	{
		return this->timeoutCount;
	}

private:
	uint64_t ComputeNextTimeoutMs() const;

	/* Pure virtual methods inherited from TimerHandle::Listener. */
public:
	void OnTimer(TimerHandle* timer) override;

private:
	// Passed by argument.
	Listener* listener{ nullptr };
	uint64_t baseTimeoutMs{ 0 };
	BackoffAlgorithm backoffAlgorithm;
	std::optional<uint64_t> maxBackoffTimeoutMs;
	std::optional<size_t> maxRestarts;
	// Allocated by this.
	TimerHandle* timer{ nullptr };
	// Others.
	bool active{ false };
	size_t timeoutCount{ 0 };
};

#endif
