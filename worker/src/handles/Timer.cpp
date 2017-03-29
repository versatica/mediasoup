#define MS_CLASS "Timer"
// #define MS_LOG_DEV

#include "handles/Timer.hpp"
#include "MediaSoupError.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

/* Static methods for UV callbacks. */

static inline
void on_timer(uv_timer_t* handle)
{
	static_cast<Timer*>(handle->data)->onUvTimer();
}

static inline
void on_close(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

Timer::Timer(Listener* listener) :
	listener(listener)
{
	MS_TRACE();

	int err;

	this->uvHandle = new uv_timer_t;
	uvHandle->data = (void*)this;

	err = uv_timer_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;
		MS_THROW_ERROR("uv_timer_init() failed: %s", uv_strerror(err));
	}
}

void Timer::Destroy()
{
	MS_TRACE();

	uv_close((uv_handle_t*)this->uvHandle, (uv_close_cb)on_close);

	// Delete this.
	delete this;
}

void Timer::Start(uint64_t timeout)
{
	MS_TRACE();

	int err;

	err = uv_timer_start(this->uvHandle, (uv_timer_cb)on_timer, timeout, 0);
	if (err)
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
}

void Timer::Stop()
{
	MS_TRACE();

	int err;

	err = uv_timer_stop(this->uvHandle);
	if (err)
		MS_THROW_ERROR("uv_timer_stop() failed: %s", uv_strerror(err));
}

inline
void Timer::onUvTimer()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->onTimer(this);
}
