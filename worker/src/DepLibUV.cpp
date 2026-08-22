#define MS_CLASS "DepLibUV"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

/* Class variables. */

thread_local uv_loop_t* DepLibUV::loop{ nullptr };
thread_local uint64_t DepLibUV::ntpOffsetMs{ 0 };

/* Static methods for UV callbacks. */

inline static void onCloseLoop(uv_handle_t* handle)
{
	MS_TRACE_STD();

	delete reinterpret_cast<uv_loop_t*>(handle);
}

inline static void onWalk(uv_handle_t* handle, void* /*arg*/)
{
	// Must use MS_ERROR_STD() since at this point the Channel is already closed.
	MS_ERROR_STD(
	  "alive UV handle found (this shouldn't happen) [type:%s, active:%d, closing:%d, has_ref:%d]",
	  uv_handle_type_name(handle->type),
	  uv_is_active(handle),
	  uv_is_closing(handle),
	  uv_has_ref(handle));

	if (!uv_is_closing(handle))
	{
		uv_close(handle, onCloseLoop);
	}
}

/* Static methods. */

void DepLibUV::ClassInit()
{
	// Must use MS_TRACE_STD() since the Channel is not yet created.
	MS_TRACE_STD();

	DepLibUV::loop = new uv_loop_t;

	const int err = uv_loop_init(DepLibUV::loop);

	if (err != 0)
	{
		MS_ABORT("libuv loop initialization failed");
	}

	// Take the distance from our own monotonic clock to the NTP epoch just once, so that
	// the NTP timestamps we generate never step when the system clock is adjusted.
	uv_timeval64_t timeval{}; // NOLINT(cppcoreguidelines-pro-type-member-init)

	if (uv_gettimeofday(std::addressof(timeval)) != 0)
	{
		MS_ABORT("uv_gettimeofday() failed");
	}

	const auto unixSec = static_cast<uint64_t>(timeval.tv_sec);
	const auto unixMs  = (unixSec * 1000) + (static_cast<uint64_t>(timeval.tv_usec) / 1000);
	const auto ntpMs   = unixMs + (static_cast<uint64_t>(Utils::Time::UnixNtpOffsetSec) * 1000);

	DepLibUV::ntpOffsetMs = ntpMs - DepLibUV::GetTimeMs();
}

void DepLibUV::ClassDestroy()
{
	MS_TRACE_STD();

	// Here we should not have any UV handle left. All them should have been
	// already closed+freed. However, in order to not introduce regressions
	// in the future, we check this anyway.
	// More context: https://github.com/versatica/mediasoup/pull/576

	int err;

	uv_stop(DepLibUV::loop);
	uv_walk(DepLibUV::loop, onWalk, nullptr);

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
	{
		MS_ERROR_STD("failed to close libuv loop: %s", uv_err_name(err));
	}

	delete DepLibUV::loop;
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

	const int ret = uv_run(DepLibUV::loop, UV_RUN_DEFAULT);

	MS_ASSERT(ret == 0, "uv_run() returned %s", uv_err_name(ret));
}
