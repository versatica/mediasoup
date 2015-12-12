#ifndef MS_DEP_LIBUV_H
#define	MS_DEP_LIBUV_H

#include <uv.h>

class DepLibUV
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static void RunLoop();
	static uv_loop_t* GetLoop();

private:
	static uv_loop_t* loop;
};

/* Inline static methods. */

inline
uv_loop_t* DepLibUV::GetLoop()
{
	return DepLibUV::loop;
}

#endif
