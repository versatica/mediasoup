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
	static uint64_t GetTimeUs()
	{
		return static_cast<uint64_t>(uv_hrtime() / 1000u);
	}
	static uint64_t GetTimeNs()
	{
		return uv_hrtime();
	}

private:
	static uv_loop_t* loop;
};

#endif
