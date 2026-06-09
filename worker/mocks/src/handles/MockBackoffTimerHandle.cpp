#define MS_CLASS "mocks::MockBackoffTimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <string>

namespace mocks
{
	MockBackoffTimerHandle::MockBackoffTimerHandle(
	  BackoffTimerHandleOptions options,
	  std::function<uint64_t()> getTimeMs,
	  std::function<void()> onDelete)
	  : listener(options.listener),
	    label(std::move(options.label)),
	    baseTimeoutMs(options.baseTimeoutMs),
	    backoffAlgorithm(options.backoffAlgorithm),
	    maxBackoffTimeoutMs(options.maxBackoffTimeoutMs),
	    maxRestarts(options.maxRestarts),
	    getTimeMs(std::move(getTimeMs)),
	    onDelete(std::move(onDelete))
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_THROW_TYPE_ERROR("options.listener must be given");
		}

		if (this->label.empty())
		{
			MS_THROW_TYPE_ERROR("options.label must be given");
		}

		if (this->baseTimeoutMs > BackoffTimerHandleInterface::MaxTimeoutMs)
		{
			MS_THROW_ERROR(
			  "[%s] base timeout (%" PRIu64 " ms) cannot be greater than %" PRIu64 " ms",
			  this->label.c_str(),
			  this->baseTimeoutMs,
			  BackoffTimerHandleInterface::MaxTimeoutMs);
		}
	}

	void MockBackoffTimerHandle::Dump(int indentation) const
	{
		MS_TRACE();

		const uint64_t nowMs = this->getTimeMs();

		MS_DUMP_CLEAN(indentation, "<mocks::MockBackoffTimerHandle>");

		MS_DUMP_CLEAN(indentation, "  label: %s", this->label.c_str());
		MS_DUMP_CLEAN(indentation, "  base timeout (ms): %" PRIu64, this->baseTimeoutMs);
		MS_DUMP_CLEAN(
		  indentation,
		  "  max backoff timeout (ms): %s",
		  this->maxBackoffTimeoutMs.has_value()
		    ? std::to_string(this->maxBackoffTimeoutMs.value()).c_str()
		    : "(unset)");
		MS_DUMP_CLEAN(
		  indentation,
		  "  max restarts (ms): %s",
		  this->maxRestarts.has_value() ? std::to_string(this->maxRestarts.value()).c_str() : "(unset)");
		MS_DUMP_CLEAN(indentation, "  running: %s", this->running ? "yes" : "no");
		MS_DUMP_CLEAN(indentation, "  expiration count: %zu", this->expirationCount);
		MS_DUMP_CLEAN(indentation, "  now (ms): %" PRIu64, nowMs);
		MS_DUMP_CLEAN(indentation, "  expires at (ms): %" PRIu64, this->expiresAtMs);
		MS_DUMP_CLEAN(indentation, "  expires in (ms): %" PRIu64, this->expiresAtMs - nowMs);

		MS_DUMP_CLEAN(indentation, "</mocks::MockBackoffTimerHandle>");
	}

	void MockBackoffTimerHandle::SetBaseTimeoutMs(uint64_t baseTimeoutMs)
	{
		MS_TRACE();

		if (baseTimeoutMs > BackoffTimerHandleInterface::MaxTimeoutMs)
		{
			MS_THROW_ERROR(
			  "[%s] base timeout (%" PRIu64 " ms) cannot be greater than %" PRIu64 " ms",
			  this->label.c_str(),
			  baseTimeoutMs,
			  BackoffTimerHandleInterface::MaxTimeoutMs);
		}

		this->baseTimeoutMs = baseTimeoutMs;
	}

	void MockBackoffTimerHandle::TriggerExpire()
	{
		MS_TRACE();

		this->expirationCount++;

		// Compute whether the BackoffTimer should still be running after this timeout
		// expiration so the parent can check IsRunning() within the `OnBackoffTimer()`
		// callback.
		this->running =
		  !this->maxRestarts.has_value() || this->expirationCount <= this->maxRestarts.value();

		uint64_t baseTimeoutMs{ this->baseTimeoutMs };
		bool stop{ false };

		// Call the listener by passing base timeout as reference so the parent has
		// a chance to change it and affect the next timeout.
		this->listener->OnBackoffTimer(this, baseTimeoutMs, stop);

		// If the parent has set `stop` to true it means that it has deleted the
		// instance, so stop here.
		if (stop)
		{
			return;
		}

		// NOTE: This may throw.
		SetBaseTimeoutMs(baseTimeoutMs);

		// The caller may have called Stop() within the callback so we must check
		// the `running` flag.
		if (this->running)
		{
			this->expiresAtMs = this->getTimeMs() + ComputeNextTimeoutMs();
		}
		// Once the timer is no longer running (e.g. max restarts reached), the real
		// BackoffTimerHandle doesn't restart the underlying timer, so it won't fire
		// again. Mirror that here so a stopped timer doesn't keep expiring.
		else
		{
			this->expiresAtMs = std::numeric_limits<uint64_t>::max();
		}
	}
} // namespace mocks
