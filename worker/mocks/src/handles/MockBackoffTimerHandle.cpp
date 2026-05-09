#define MS_CLASS "mocks::MockBackoffTimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "Logger.hpp"
#include <string>

namespace mocks
{
	void MockBackoffTimerHandle::Dump(int indentation) const
	{
		MS_TRACE();

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
		MS_DUMP_CLEAN(indentation, "  now (ms): %" PRIu64, this->getTimeMs());
		MS_DUMP_CLEAN(indentation, "  expires at (ms): %" PRIu64, this->expiresAtMs);

		MS_DUMP_CLEAN(indentation, "</mocks::MockBackoffTimerHandle>");
	}
} // namespace mocks
