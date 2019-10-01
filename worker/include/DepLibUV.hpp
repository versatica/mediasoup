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
	static uv_loop_t* GetLoop();
	static uint64_t GetTimeMs();
	static uint64_t GetTimeUs();
	static uint64_t GetTimeNs();

private:
	static uv_loop_t* loop;
};

/* Inline static methods. */

inline uv_loop_t* DepLibUV::GetLoop()
{
	return DepLibUV::loop;
}

inline uint64_t DepLibUV::GetTimeMs()
{
	return static_cast<uint64_t>(uv_hrtime() / 1000000u);
}

inline uint64_t DepLibUV::GetTimeUs()
{
	return static_cast<uint64_t>(uv_hrtime() / 1000u);
}

inline uint64_t DepLibUV::GetTimeNs()
{
	return uv_hrtime();
}

#endif
