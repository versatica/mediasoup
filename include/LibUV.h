#ifndef MS_LIBUV_H
#define	MS_LIBUV_H


#include "common.h"
#include <uv.h>


class LibUV {
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
uv_loop_t* LibUV::GetLoop() {
	return LibUV::loop;
}


#endif
