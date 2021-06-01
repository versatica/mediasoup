#define MS_CLASS "DepLibUV"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <cstdlib> // std::abort()

/* Static variables. */

thread_local uv_loop_t* DepLibUV::loop{ nullptr };

void on_uv_close(uv_handle_t* handle)
{
	if (handle != NULL)
	{
		delete handle;
	}
}

void on_uv_walk(uv_handle_t* handle, void* arg)
{
	if (!uv_is_closing(handle))
		uv_close(handle, on_uv_close);
}

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
		int err;

		uv_stop(DepLibUV::loop);
		uv_walk(DepLibUV::loop, on_uv_walk, NULL);

		while (true)
		{
			err = uv_loop_close(DepLibUV::loop);

			if (err != UV_EBUSY)
			{
				break;
			}

			uv_run(DepLibUV::loop, UV_RUN_NOWAIT);
		}

		if (err != 0)
			MS_ABORT("failed to close libuv loop: %s", uv_err_name(err));

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
