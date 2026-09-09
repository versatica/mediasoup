#define MS_CLASS "mocks::MockTimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/handles/MockTimerHandle.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace mocks
{
	MockTimerHandle::MockTimerHandle(
	  TimerHandleInterface::Listener* listener,
	  std::string label,
	  std::function<int64_t()> getTimeMs,
	  std::function<void()> onDelete)
	  : listener(listener),
	    label(std::move(label)),
	    getTimeMs(std::move(getTimeMs)),
	    onDelete(std::move(onDelete))
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_THROW_TYPE_ERROR("[%s] listener must be given", this->label.c_str());
		}

		if (this->label.empty())
		{
			MS_THROW_TYPE_ERROR("label must be given");
		}
	}

	void MockTimerHandle::Dump(int indentation) const
	{
		MS_TRACE();

		const int64_t nowMs = this->getTimeMs();

		MS_DUMP_CLEAN(indentation, "<mocks::MockTimerHandle>");

		MS_DUMP_CLEAN(indentation, "  label: %s", this->label.c_str());
		MS_DUMP_CLEAN(indentation, "  timeout (ms): %" PRIi64, this->timeoutMs);
		MS_DUMP_CLEAN(indentation, "  repeat (ms): %" PRIi64, this->repeatMs);
		MS_DUMP_CLEAN(indentation, "  running: %s", this->running ? "yes" : "no");
		MS_DUMP_CLEAN(indentation, "  now (ms): %" PRIi64, nowMs);
		MS_DUMP_CLEAN(indentation, "  expires at (ms): %" PRIi64, this->expiresAtMs);
		MS_DUMP_CLEAN(indentation, "  expires in (ms): %" PRIi64, this->expiresAtMs - nowMs);

		MS_DUMP_CLEAN(indentation, "</mocks::MockTimerHandle>");
	}

	void MockTimerHandle::Start(int64_t timeoutMs, int64_t repeatMs)
	{
		MS_TRACE();

		if (timeoutMs < 0)
		{
			MS_THROW_TYPE_ERROR(
			  "[%s] timeoutMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), timeoutMs);
		}

		if (repeatMs < 0)
		{
			MS_THROW_TYPE_ERROR(
			  "[%s] repeatMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), repeatMs);
		}

		this->timeoutMs = timeoutMs;
		this->repeatMs  = repeatMs;

		this->running     = true;
		this->expiresAtMs = this->getTimeMs() + this->timeoutMs;
	}

	void MockTimerHandle::Restart(int64_t timeoutMs, int64_t repeatMs)
	{
		MS_TRACE();

		if (timeoutMs < 0)
		{
			MS_THROW_TYPE_ERROR(
			  "[%s] timeoutMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), timeoutMs);
		}

		if (repeatMs < 0)
		{
			MS_THROW_TYPE_ERROR(
			  "[%s] repeatMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), repeatMs);
		}

		Start(timeoutMs, repeatMs);
	}

	void MockTimerHandle::TriggerExpire()
	{
		MS_TRACE();

		// Schedule the next expiration, or deactivate the timer if it doesn't
		// repeat, just like the real TimerHandle does. This is done before invoking
		// the callback for two reasons: the listener may call Start(), Restart() or
		// Stop() within it, which must win over this, and the listener may delete
		// this instance within it, so nothing can be accessed afterwards.
		if (this->repeatMs != 0)
		{
			this->running     = true;
			this->expiresAtMs = this->getTimeMs() + this->repeatMs;
		}
		else
		{
			this->running     = false;
			this->expiresAtMs = std::numeric_limits<int64_t>::max();
		}

		// Notify the listener.
		this->listener->OnTimer(this);
	}
} // namespace mocks
