#define MS_CLASS "DepLibUV"
#define MS_LOG_DEV

#include "DepLibUV.h"
#include "Logger.h"
#include <cstdlib> // std::abort()

/* Static variables. */

uv_loop_t* DepLibUV::loop = nullptr;
uint32_t DepLibUV::timeUpdateCounter = 0;
static uint64_t LAST_NOW = 0;

/* Static methods. */

void DepLibUV::ClassInit()
{
	// NOTE: Logger depends on this so we cannot log anything here.

	int err;

	DepLibUV::loop = new uv_loop_t;
	err = uv_loop_init(DepLibUV::loop);
	if (err)
		MS_ABORT("libuv initialization failed");

	LAST_NOW = uv_now(DepLibUV::loop);
}

void DepLibUV::ClassDestroy()
{
	MS_TRACE();

	// This should never happen.
	if (!DepLibUV::loop)
		MS_ABORT("DepLibUV::loop was not allocated");

	uv_loop_close(DepLibUV::loop);
	delete DepLibUV::loop;
}

void DepLibUV::PrintVersion()
{
	MS_TRACE();

	MS_DEBUG_TAG(info, "loaded libuv version: %s", uv_version_string());
}

void DepLibUV::RunLoop()
{
	MS_TRACE();

	// This should never happen.
	if (!DepLibUV::loop)
		MS_ABORT("DepLibUV::loop was not allocated");

	uv_run(DepLibUV::loop, UV_RUN_DEFAULT);
}

// TODO: Move to inline in the .h file.
uint64_t DepLibUV::GetTime()
{
	static uint32_t maxTimeCounter = 32768;

	// TODO: Remove.
	if (DepLibUV::timeUpdateCounter == 0 && LAST_NOW != 0)
		MS_DEBUG_DEV("now - LAST_NOW: %" PRIu64 "ms", uv_now(DepLibUV::loop) - LAST_NOW);

	// Update the libuv's concept of “now” every N usages.
	if (++DepLibUV::timeUpdateCounter == maxTimeCounter)
	{
		LAST_NOW = uv_now(DepLibUV::loop);
		DepLibUV::timeUpdateCounter = 0;
		uv_update_time(DepLibUV::loop);
	}

	return uv_now(DepLibUV::loop);
}
