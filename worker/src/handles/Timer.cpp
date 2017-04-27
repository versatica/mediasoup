#define MS_CLASS "Timer"
// #define MS_LOG_DEV

#include "handles/Timer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"

/* Static methods for UV callbacks. */

inline static void onTimer(uv_timer_t* handle)
{
	static_cast<Timer*>(handle->data)->OnUvTimer();
}

inline static void onClose(uv_handle_t* handle)
{
	delete handle;
}

/* Instance methods. */

Timer::Timer(Listener* listener) : listener(listener)
{
	MS_TRACE();

	int err;

	this->uvHandle = new uv_timer_t;
	uvHandle->data = (void*)this;

	err = uv_timer_init(DepLibUV::GetLoop(), this->uvHandle);
	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_timer_init() failed: %s", uv_strerror(err));
	}
}

void Timer::Destroy()
{
	MS_TRACE();

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));

	// Delete this.
	delete this;
}

void Timer::Start(uint64_t timeout, uint64_t repeat)
{
	MS_TRACE();

	int err;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
		Stop();

	err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), timeout, repeat);
	if (err != 0)
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
}

void Timer::Stop()
{
	MS_TRACE();

	int err;

	err = uv_timer_stop(this->uvHandle);
	if (err != 0)
		MS_THROW_ERROR("uv_timer_stop() failed: %s", uv_strerror(err));
}

inline void Timer::OnUvTimer()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->OnTimer(this);
}
