#define MS_CLASS "TimerHandle"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/TimerHandle.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

/* Static methods for UV callbacks. */

static void onTimer(uv_timer_t* handle)
{
	MS_TRACE();

	static_cast<TimerHandle*>(handle->data)->OnUvTimer();
}

static void onCloseTimer(uv_handle_t* handle)
{
	MS_TRACE();

	delete reinterpret_cast<uv_timer_t*>(handle);
}

/* Instance methods. */

TimerHandle::TimerHandle(TimerHandleInterface::Listener* listener, std::string label)
  : listener(listener), label(std::move(label)), uvHandle(new uv_timer_t)
{
	MS_TRACE();

	if (!this->listener)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_TYPE_ERROR("[%s] listener must be given", this->label.c_str());
	}

	if (this->label.empty())
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_TYPE_ERROR("label must be given");
	}

	this->uvHandle->data = static_cast<void*>(this);

	const int err = uv_timer_init(DepLibUV::GetLoop(), this->uvHandle);

	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("[%s] uv_timer_init() failed: %s", this->label.c_str(), uv_strerror(err));
	}
}

TimerHandle::~TimerHandle()
{
	MS_TRACE();

	if (!this->closed)
	{
		InternalClose();
	}
}

void TimerHandle::Start(int64_t timeoutMs, int64_t repeatMs)
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("[%s] closed", this->label.c_str());
	}

	if (timeoutMs < 0)
	{
		MS_THROW_TYPE_ERROR(
		  "[%s] timeoutMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), timeoutMs);
	}

	if (repeatMs < 0)
	{
		MS_THROW_TYPE_ERROR(
		  "[%s] repeatMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), repeatMs);
	}

	this->timeoutMs = timeoutMs;
	this->repeatMs  = repeatMs;

	int err;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		err = uv_timer_stop(this->uvHandle);

		if (err != 0)
		{
			MS_THROW_ERROR("[%s] uv_timer_stop() failed: %s", this->label.c_str(), uv_strerror(err));
		}
	}

	err = uv_timer_start(
	  this->uvHandle,
	  static_cast<uv_timer_cb>(onTimer),
	  static_cast<uint64_t>(this->timeoutMs),
	  static_cast<uint64_t>(this->repeatMs));

	if (err != 0)
	{
		MS_THROW_ERROR("[%s] uv_timer_start() failed: %s", this->label.c_str(), uv_strerror(err));
	}
}

void TimerHandle::Stop()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("[%s] closed", this->label.c_str());
	}

	const int err = uv_timer_stop(this->uvHandle);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_stop() failed: %s", uv_strerror(err));
	}
}

void TimerHandle::Restart()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("[%s] closed", this->label.c_str());
	}

	int err;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		err = uv_timer_stop(this->uvHandle);

		if (err != 0)
		{
			MS_THROW_ERROR("[%s] uv_timer_stop() failed: %s", this->label.c_str(), uv_strerror(err));
		}
	}

	err = uv_timer_start(
	  this->uvHandle,
	  static_cast<uv_timer_cb>(onTimer),
	  static_cast<uint64_t>(this->timeoutMs),
	  static_cast<uint64_t>(this->repeatMs));

	if (err != 0)
	{
		MS_THROW_ERROR("[%s] uv_timer_start() failed: %s", this->label.c_str(), uv_strerror(err));
	}
}

void TimerHandle::Restart(int64_t timeoutMs, int64_t repeatMs)
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("[%s] closed", this->label.c_str());
	}

	if (timeoutMs < 0)
	{
		MS_THROW_TYPE_ERROR(
		  "[%s] timeoutMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), timeoutMs);
	}

	if (repeatMs < 0)
	{
		MS_THROW_TYPE_ERROR(
		  "[%s] repeatMs (%" PRIi64 " ms) cannot be negative", this->label.c_str(), repeatMs);
	}

	this->timeoutMs = timeoutMs;
	this->repeatMs  = repeatMs;

	int err;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		err = uv_timer_stop(this->uvHandle);

		if (err != 0)
		{
			MS_THROW_ERROR("[%s] uv_timer_stop() failed: %s", this->label.c_str(), uv_strerror(err));
		}
	}

	err = uv_timer_start(
	  this->uvHandle,
	  static_cast<uv_timer_cb>(onTimer),
	  static_cast<uint64_t>(this->timeoutMs),
	  static_cast<uint64_t>(this->repeatMs));

	if (err != 0)
	{
		MS_THROW_ERROR("[%s] uv_timer_start() failed: %s", this->label.c_str(), uv_strerror(err));
	}
}

void TimerHandle::InternalClose()
{
	MS_TRACE();

	if (this->closed)
	{
		return;
	}

	this->closed = true;

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onCloseTimer));
}

void TimerHandle::OnUvTimer()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->OnTimer(this);
}
