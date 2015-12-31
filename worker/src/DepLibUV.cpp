#define MS_CLASS "DepLibUV"

#include "DepLibUV.h"
#include "Logger.h"
#include <cstdlib>  // std::abort()

/* Static variables. */

uv_loop_t* DepLibUV::loop = nullptr;

/* Static methods. */

void DepLibUV::ClassInit()
{
	// NOTE: Logger depends on this so we cannot log anything here.

	int err;

	DepLibUV::loop = new uv_loop_t;
	err = uv_loop_init(DepLibUV::loop);
	if (err)
		std::abort();
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

	MS_DEBUG("loaded libuv version: %s", uv_version_string());
}

void DepLibUV::RunLoop()
{
	MS_TRACE();

	// This should never happen.
	if (!DepLibUV::loop)
		MS_ABORT("DepLibUV::loop was not allocated");

	uv_run(DepLibUV::loop, UV_RUN_DEFAULT);
}
