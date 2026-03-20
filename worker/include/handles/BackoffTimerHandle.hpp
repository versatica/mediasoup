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
		 * - If the caller deletes this BackoffTimer instance within the callback
		 *   it must signal it be setting `stop` to true.
		 */
		virtual void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) = 0;
	};

public:
	enum class BackoffAlgorithm : uint8_t
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

	~BackoffTimerHandle() override;

public:
	/**
	 * Start the BackoffTimer (if it's stopped) or restart it (if already
	 * running). It will reset the timeout count.
	 */
	void Start();

	/**
	 * Stop the BackoffTimer. It will reset the timeout count.
	 */
	void Stop();

	/**
	 * Set the base timeout duration. It will be applied after the next timeout
	 * and effective duration can be larger if backoff algorithm is exponential.
	 */
	void SetBaseTimeoutMs(uint64_t baseTimeoutMs);

	/**
	 * Whether the BackoffTimer is running. Useful to check if this BackoffTimer
	 * will timeout again within the OnTimer() callback.
	 */
	bool IsRunning() const
	{
		return this->running;
	}

	/**
	 * Maximum number of restarts.
	 *
	 * @remarks
	 * - If `maxRestarts` was not given in the constructor, this method returns 0.
	 */
	std::optional<size_t> GetMaxRestarts() const
	{
		return this->maxRestarts;
	}

	/**
	 * Number of times the timer has expired.
	 */
	size_t GetExpirationCount() const
	{
		return this->expirationCount;
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
	bool running{ false };
	size_t expirationCount{ 0 };
};

#endif
