#define MS_CLASS "DepLibUV"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <cstdlib> // std::abort()

/* Static variables. */

uv_loop_t* DepLibUV::loop{ nullptr };

/* Static methods. */

void DepLibUV::ClassInit()
{
	// NOTE: Logger depends on this so we cannot log anything here.

	DepLibUV::loop = new uv_loop_t;

	int err = uv_loop_init(DepLibUV::loop);

	if (err != 0)
		MS_ABORT("libuv initialization failed");
}

void DepLibUV::ClassDestroy()
{
	MS_TRACE();

	// This should never happen.
	if (DepLibUV::loop != nullptr)
	{
		uv_loop_close(DepLibUV::loop);
		delete DepLibUV::loop;
	}
}

void DepLibUV::PrintVersion()
{
	MS_TRACE();

	MS_DEBUG_TAG(info, "libuv version: \"%s\"", uv_version_string());
}

void DepLibUV::RunLoop()
{
	MS_TRACE();

	// This should never happen.
	MS_ASSERT(DepLibUV::loop != nullptr, "loop unset");

	uv_run(DepLibUV::loop, UV_RUN_DEFAULT);
}
