#define MS_CLASS "BackoffTimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/BackoffTimerHandle.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <algorithm> // std::min()

/* Instance methods. */

BackoffTimerHandle::BackoffTimerHandle(
  Listener* listener,
  uint64_t baseTimeout,
  BackoffAlgorithm backoffAlgorithm,
  std::optional<uint64_t> maxBackoffTimeout,
  std::optional<size_t> maxRestarts)
  : listener(listener), baseTimeout(baseTimeout), backoffAlgorithm(backoffAlgorithm),
    maxBackoffTimeout(maxBackoffTimeout), maxRestarts(maxRestarts)
{
	MS_TRACE();

	// NOTE: This may throw.
	SetBaseTimeout(baseTimeout);

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

	this->timer->Start(this->baseTimeout);

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

void BackoffTimerHandle::SetBaseTimeout(uint64_t baseTimeout)
{
	MS_TRACE();

	if (baseTimeout > BackoffTimerHandle::MaxTimeout)
	{
		MS_THROW_ERROR(
		  "base timeout (%" PRIu64 ") cannot be greater than %" PRIu64,
		  baseTimeout,
		  BackoffTimerHandle::MaxTimeout);
	}

	this->baseTimeout = baseTimeout;
}

uint64_t BackoffTimerHandle::ComputeNextTimeout() const
{
	MS_TRACE();

	auto timeoutCount = this->timeoutCount;

	switch (this->backoffAlgorithm)
	{
		case BackoffAlgorithm::FIXED:
		{
			return this->baseTimeout;
		}

		case BackoffAlgorithm::EXPONENTIAL:
		{
			auto timeout = this->baseTimeout;

			while (timeoutCount > 0 && timeout < BackoffTimerHandle::MaxTimeout)
			{
				timeout *= 2;
				--timeoutCount;

				if (this->maxBackoffTimeout.has_value() && timeout > *this->maxBackoffTimeout)
				{
					return *this->maxBackoffTimeout;
				}
			}

			return std::min<uint64_t>(timeout, BackoffTimerHandle::MaxTimeout);
		}
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

	uint64_t baseTimeout{ this->baseTimeout };
	bool stop{ false };

	// Call the listener by passing base timeout as reference so the parent has
	// a chance to change it and affect the next timeout.
	this->listener->OnTimer(this, baseTimeout, stop);

	// If the parent has set `stop` to true it means that it has deleted the
	// instance, so stop here.
	if (stop)
	{
		return;
	}

	// NOTE: This may throw.
	SetBaseTimeout(baseTimeout);

	// The caller may have called Stop() within the callback so we must check
	// the `active` flag.
	if (this->active)
	{
		auto nextTimeout = ComputeNextTimeout();

		this->timer->Start(nextTimeout);
	}
}
