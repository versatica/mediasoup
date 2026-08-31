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
	  std::function<uint64_t()> getTimeMs,
	  std::function<void()> onDelete)
	  : listener(listener),
	    label(std::move(label)),
	    getTimeMs(std::move(getTimeMs)),
	    onDelete(std::move(onDelete))
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_THROW_TYPE_ERROR("listener must be given");
		}

		if (this->label.empty())
		{
			MS_THROW_TYPE_ERROR("label must be given");
		}
	}

	void MockTimerHandle::Dump(int indentation) const
	{
		MS_TRACE();

		const uint64_t nowMs = this->getTimeMs();

		MS_DUMP_CLEAN(indentation, "<mocks::MockTimerHandle>");

		MS_DUMP_CLEAN(indentation, "  label: %s", this->label.c_str());
		MS_DUMP_CLEAN(indentation, "  timeout (ms): %" PRIu64, this->timeout);
		MS_DUMP_CLEAN(indentation, "  repeat (ms): %" PRIu64, this->repeat);
		MS_DUMP_CLEAN(indentation, "  running: %s", this->running ? "yes" : "no");
		MS_DUMP_CLEAN(indentation, "  now (ms): %" PRIu64, nowMs);
		MS_DUMP_CLEAN(indentation, "  expires at (ms): %" PRIu64, this->expiresAtMs);
		MS_DUMP_CLEAN(indentation, "  expires in (ms): %" PRIu64, this->expiresAtMs - nowMs);

		MS_DUMP_CLEAN(indentation, "</mocks::MockTimerHandle>");
	}

	void MockTimerHandle::TriggerExpire()
	{
		MS_TRACE();

		// Schedule the next expiration, or deactivate the timer if it doesn't
		// repeat, just like the real TimerHandle does. This is done before invoking
		// the callback for two reasons: the listener may call Start(), Restart() or
		// Stop() within it, which must win over this, and the listener may delete
		// this instance within it, so nothing can be accessed afterwards.
		if (this->repeat != 0)
		{
			this->running     = true;
			this->expiresAtMs = this->getTimeMs() + this->repeat;
		}
		else
		{
			this->running     = false;
			this->expiresAtMs = std::numeric_limits<uint64_t>::max();
		}

		// Notify the listener.
		this->listener->OnTimer(this);
	}
} // namespace mocks
