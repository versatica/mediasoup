#define MS_CLASS "DepLibUV"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

/* Class variables. */

thread_local std::unique_ptr<uv_loop_t> DepLibUV::loop;
thread_local int64_t DepLibUV::ntpOffsetUs{ 0 };

/* Static methods for UV callbacks. */

inline static void onCloseWalkedHandle(uv_handle_t* handle)
{
	MS_TRACE_STD();

	// The handle must be deleted with the same type it was allocated with.
	switch (handle->type)
	{
		case UV_ASYNC:
		{
			delete reinterpret_cast<uv_async_t*>(handle);

			break;
		}

		case UV_CHECK:
		{
			delete reinterpret_cast<uv_check_t*>(handle);

			break;
		}

		case UV_FS_EVENT:
		{
			delete reinterpret_cast<uv_fs_event_t*>(handle);

			break;
		}

		case UV_FS_POLL:
		{
			delete reinterpret_cast<uv_fs_poll_t*>(handle);

			break;
		}

		case UV_HANDLE:
		{
			delete handle;

			break;
		}

		case UV_IDLE:
		{
			delete reinterpret_cast<uv_idle_t*>(handle);

			break;
		}

		case UV_NAMED_PIPE:
		{
			delete reinterpret_cast<uv_pipe_t*>(handle);

			break;
		}

		case UV_POLL:
		{
			delete reinterpret_cast<uv_poll_t*>(handle);

			break;
		}

		case UV_PREPARE:
		{
			delete reinterpret_cast<uv_prepare_t*>(handle);

			break;
		}

		case UV_PROCESS:
		{
			delete reinterpret_cast<uv_process_t*>(handle);

			break;
		}

		case UV_STREAM:
		{
			delete reinterpret_cast<uv_stream_t*>(handle);

			break;
		}

		case UV_TCP:
		{
			delete reinterpret_cast<uv_tcp_t*>(handle);

			break;
		}

		case UV_TIMER:
		{
			delete reinterpret_cast<uv_timer_t*>(handle);

			break;
		}

		case UV_TTY:
		{
			delete reinterpret_cast<uv_tty_t*>(handle);

			break;
		}

		case UV_UDP:
		{
			delete reinterpret_cast<uv_udp_t*>(handle);

			break;
		}

		case UV_SIGNAL:
		{
			delete reinterpret_cast<uv_signal_t*>(handle);

			break;
		}

		default:
		{
			// Must use MS_ERROR_STD() since at this point the Channel is already closed.
			MS_ERROR_STD(
			  "cannot free UV handle of unknown type, leaking it [type:%d]",
			  static_cast<int>(handle->type));
		}
	}
}

inline static void onWalk(uv_handle_t* handle, void* /*arg*/)
{
	MS_TRACE_STD();

	// Must use MS_ERROR_STD() since at this point the Channel is already closed.
	MS_ERROR_STD(
	  "alive UV handle found (this shouldn't happen) [type:%s, active:%d, closing:%d, has_ref:%d]",
	  uv_handle_type_name(handle->type),
	  uv_is_active(handle),
	  uv_is_closing(handle),
	  uv_has_ref(handle));

	if (!uv_is_closing(handle))
	{
		uv_close(handle, onCloseWalkedHandle);
	}
}

/* Static methods. */

void DepLibUV::ClassInit()
{
	// Must use MS_TRACE_STD() since the Channel is not yet created.
	MS_TRACE_STD();

	DepLibUV::loop = std::make_unique<uv_loop_t>();

	const int err = uv_loop_init(DepLibUV::loop.get());

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

	const int64_t unixUs = (timeval.tv_sec * 1000000) + static_cast<int64_t>(timeval.tv_usec);
	const int64_t ntpUs  = unixUs + (static_cast<int64_t>(Utils::Time::UnixNtpOffsetSec) * 1000000);

	DepLibUV::ntpOffsetUs = ntpUs - DepLibUV::GetTimeUs();
}

void DepLibUV::ClassDestroy()
{
	MS_TRACE_STD();

	// Here we should not have any UV handle left. All them should have been
	// already closed+freed. However, in order to not introduce regressions
	// in the future, we check this anyway.
	// More context: https://github.com/versatica/mediasoup/pull/576

	int err;

	uv_stop(DepLibUV::loop.get());
	uv_walk(DepLibUV::loop.get(), onWalk, nullptr);

	while (true)
	{
		err = uv_loop_close(DepLibUV::loop.get());

		if (err != UV_EBUSY)
		{
			break;
		}

		uv_run(DepLibUV::loop.get(), UV_RUN_NOWAIT);
	}

	if (err != 0)
	{
		MS_ERROR_STD("failed to close libuv loop: %s", uv_err_name(err));
	}

	DepLibUV::loop.reset();
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

	const int ret = uv_run(DepLibUV::loop.get(), UV_RUN_DEFAULT);

	MS_ASSERT(ret == 0, "uv_run() returned %s", uv_err_name(ret));
}
