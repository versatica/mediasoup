#ifndef MS_DEP_LIBUV_HPP
#define MS_DEP_LIBUV_HPP

#include "common.hpp"
#include <uv.h>

class DepLibUV
{
public:
	static void ClassInit();

	static void ClassDestroy();

	static void PrintVersion();

	static void RunLoop();

	static uv_loop_t* GetLoop()
	{
		return DepLibUV::loop;
	}

	static uint64_t GetTimeMs()
	{
		return static_cast<uint64_t>(uv_hrtime() / 1000000u);
	}

	/**
	 * Offset between our own monotonic clock and the NTP epoch (ms).
	 *
	 * @remarks
	 * - It is taken just once, in ClassInit(), so that the NTP timestamps we generate
	 *   never step when the system clock is adjusted. They just drift away from the real
	 *   wall clock as much as our monotonic clock does.
	 */
	static uint64_t GetNtpOffsetMs()
	{
		return DepLibUV::ntpOffsetMs;
	}

	static uint64_t GetTimeUs()
	{
		return static_cast<uint64_t>(uv_hrtime() / 1000u);
	}

	static uint64_t GetTimeNs()
	{
		return uv_hrtime();
	}

	/**
	 * Used within libwebrtc dependency which uses int64_t values for time
	 * representation.
	 */
	static int64_t GetTimeMsInt64()
	{
		return static_cast<int64_t>(DepLibUV::GetTimeMs());
	}

	/**
	 * Used within libwebrtc dependency which uses int64_t values for time
	 * representation.
	 */
	static int64_t GetTimeUsInt64()
	{
		return static_cast<int64_t>(DepLibUV::GetTimeUs());
	}

private:
	static thread_local uv_loop_t* loop;
	// Distance from our own monotonic clock to the NTP epoch (ms), taken at
	// ClassInit().
	static thread_local uint64_t ntpOffsetMs;
};

#endif
