#ifndef MS_DEP_LIBUV_H
#define	MS_DEP_LIBUV_H

#include "common.h"
#include <uv.h>

class DepLibUV
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static void PrintVersion();
	static void RunLoop();
	static uv_loop_t* GetLoop();
	static uint64_t GetTime();

private:
	static uv_loop_t* loop;
	static uint32_t timeUpdateCounter;
};

/* Inline static methods. */

inline
uv_loop_t* DepLibUV::GetLoop()
{
	return DepLibUV::loop;
}

#endif
