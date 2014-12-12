#define MS_CLASS "LibUV"

#include "LibUV.h"
#include "Logger.h"


/* Static variables. */

__thread uv_loop_t* LibUV::loop = nullptr;


/* Static methods. */

void LibUV::ClassInit() {
	MS_TRACE();

	// Print libuv version.
	MS_DEBUG("loaded libuv version: %s", uv_version_string());
}


void LibUV::ThreadInit() {
	MS_TRACE();

	int err;

	// This should never happen.
	if (LibUV::loop)
		MS_ABORT("LibUV::loop alread allocated in this thread");

	LibUV::loop = new uv_loop_t;
	err = uv_loop_init(LibUV::loop);
	if (err)
		MS_ABORT("uv_loop_init() failed: %s", uv_strerror(err));
}


void LibUV::ThreadDestroy() {
	MS_TRACE();

	// This should never happen.
	if (! LibUV::loop)
		MS_ABORT("LibUV::loop was not allocated in this thread");

	uv_loop_close(LibUV::loop);
	delete LibUV::loop;
}


void LibUV::RunLoop() {
	MS_TRACE();

	// This should never happen.
	if (! LibUV::loop)
		MS_ABORT("LibUV::loop was not allocated in this thread");

	uv_run(LibUV::loop, UV_RUN_DEFAULT);
}
