#ifndef MS_DEP_LIBUV_H
#define	MS_DEP_LIBUV_H

#include "common.h"
#include <uv.h>

class DepLibUV
{
public:
	static void ClassInit();
	static void ThreadInit();
	static void ThreadDestroy();
	static uv_loop_t* GetLoop();
	static void RunLoop();

private:
	static __thread uv_loop_t* loop;
};

/* Inline static methods. */

inline
uv_loop_t* DepLibUV::GetLoop()
{
	return DepLibUV::loop;
}

#endif
