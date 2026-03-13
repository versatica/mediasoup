#define MS_CLASS "BackoffTimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/BackoffTimerHandle.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <algorithm> // std::min()

/* Instance methods. */

BackoffTimerHandle::BackoffTimerHandle(
  Listener* listener,
  uint64_t baseTimeoutMs,
  BackoffAlgorithm backoffAlgorithm,
  std::optional<uint64_t> maxBackoffTimeoutMs,
  std::optional<size_t> maxRestarts)
  : listener(listener), baseTimeoutMs(baseTimeoutMs), backoffAlgorithm(backoffAlgorithm),
    maxBackoffTimeoutMs(maxBackoffTimeoutMs), maxRestarts(maxRestarts)
{
	MS_TRACE();

	// NOTE: This may throw.
	SetBaseTimeoutMs(baseTimeoutMs);

	this->timer = new TimerHandle(this);
}

BackoffTimerHandle::~BackoffTimerHandle()
{
	MS_TRACE();

	delete this->timer;
	this->timer = nullptr;
}

void BackoffTimerHandle::Start()
{
	MS_TRACE();

	this->timer->Start(this->baseTimeoutMs);

	this->active       = true;
	this->timeoutCount = 0;
}

void BackoffTimerHandle::Stop()
{
	MS_TRACE();

	this->timer->Stop();

	this->active       = false;
	this->timeoutCount = 0;
}

void BackoffTimerHandle::Restart()
{
	MS_TRACE();

	this->timer->Restart();

	this->active       = true;
	this->timeoutCount = 0;
}

void BackoffTimerHandle::SetBaseTimeoutMs(uint64_t baseTimeoutMs)
{
	MS_TRACE();

	if (baseTimeoutMs > BackoffTimerHandle::MaxTimeoutMs)
	{
		MS_THROW_ERROR(
		  "base timeout (%" PRIu64 " ms) cannot be greater than %" PRIu64 " ms",
		  baseTimeoutMs,
		  BackoffTimerHandle::MaxTimeoutMs);
	}

	this->baseTimeoutMs = baseTimeoutMs;
}

uint64_t BackoffTimerHandle::ComputeNextTimeoutMs() const
{
	MS_TRACE();

	auto timeoutCount = this->timeoutCount;

	switch (this->backoffAlgorithm)
	{
		case BackoffAlgorithm::FIXED:
		{
			return this->baseTimeoutMs;
		}

		case BackoffAlgorithm::EXPONENTIAL:
		{
			auto timeoutMs = this->baseTimeoutMs;

			while (timeoutCount > 0 && timeoutMs < BackoffTimerHandle::MaxTimeoutMs)
			{
				timeoutMs *= 2;
				--timeoutCount;

				if (this->maxBackoffTimeoutMs.has_value() && timeoutMs > *this->maxBackoffTimeoutMs)
				{
					return *this->maxBackoffTimeoutMs;
				}
			}

			return std::min<uint64_t>(timeoutMs, BackoffTimerHandle::MaxTimeoutMs);
		}

			NO_DEFAULT_GCC();
	}
}

void BackoffTimerHandle::OnTimer(TimerHandle* timer)
{
	MS_TRACE();

	this->timeoutCount++;

	// Compute whether the smart timer should still be running after this timeout
	// expiration so the parent can check IsActive() within the OnTimer()
	// callback.
	this->active = !this->maxRestarts.has_value() || this->timeoutCount <= *this->maxRestarts;

	uint64_t baseTimeoutMs{ this->baseTimeoutMs };
	bool stop{ false };

	// Call the listener by passing base timeout as reference so the parent has
	// a chance to change it and affect the next timeout.
	this->listener->OnTimer(this, baseTimeoutMs, stop);

	// If the parent has set `stop` to true it means that it has deleted the
	// instance, so stop here.
	if (stop)
	{
		return;
	}

	// NOTE: This may throw.
	SetBaseTimeoutMs(baseTimeoutMs);

	// The caller may have called Stop() within the callback so we must check
	// the `active` flag.
	if (this->active)
	{
		auto nextTimeoutMs = ComputeNextTimeoutMs();

		this->timer->Start(nextTimeoutMs);
	}
}
