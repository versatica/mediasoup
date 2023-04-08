#define MS_CLASS "Timer"
// #define MS_LOG_DEV_LEVEL 3

#include "handles/Timer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

#ifdef FAKE_TIMERS

#include <map>

int64_t FakeTimerManager::now =
  1; // Don't start in 0 to avoid problems with timers that are set to run in 0.
uint16_t FakeTimerManager::nextTimerId = 0;
std::map<uint16_t, FakeTimerManager::Timer> FakeTimerManager::timers;

uint16_t FakeTimerManager::GetNexTimerId()
{
	return FakeTimerManager::nextTimerId++;
}

void FakeTimerManager::StartTimer(
  uint16_t timerId, uint64_t timeout, uint64_t repeat, std::function<void()> cb)
{
	FakeTimerManager::timers[timerId] = { FakeTimerManager::now + timeout, repeat, cb };
}

void FakeTimerManager::StopTimer(uint16_t timerId)
{
	FakeTimerManager::timers.erase(timerId);
}

bool FakeTimerManager::IsActive(uint16_t timerId)
{
	return FakeTimerManager::timers.find(timerId) != FakeTimerManager::timers.end();
}

int64_t FakeTimerManager::NextTimerTime()
{
	int64_t nextTime = -1;

	for (auto& kv : FakeTimerManager::timers)
	{
		if (nextTime == -1 || kv.second.nextTime < nextTime)
		{
			nextTime = kv.second.nextTime;
		}
	}

	return nextTime;
}

void FakeTimerManager::RunPending(int64_t now)
{
	FakeTimerManager::now = now;

	auto copy = FakeTimerManager::timers;
	for (auto& kv : copy)
	{
		if (kv.second.nextTime <= now)
		{
			auto it             = FakeTimerManager::timers.find(kv.first);
			it->second.nextTime = kv.second.repeat != 0u ? now + kv.second.repeat : -1;
			kv.second.cb();
		}
	}
}

void FakeTimerManager::RunLoop(int64_t maxTime)
{
	while (true)
	{
		uint64_t nextTime = FakeTimerManager::NextTimerTime();

		if (nextTime == -1 || (maxTime != -1 && nextTime > maxTime))
		{
			break;
		}

		FakeTimerManager::RunPending(nextTime);
	}
}

Timer::Timer(Listener* listener) : listener(listener)
{
	this->timerId = FakeTimerManager::GetNexTimerId();
}

Timer::~Timer()
{
	MS_TRACE();

	if (!this->closed)
	{
		Close();
	}
}

void Timer::Close()
{
	MS_TRACE();

	if (this->closed)
	{
		return;
	}

	Stop();

	this->closed = true;
}

void Timer::Start(uint64_t timeout, uint64_t repeat)
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	this->timeout = timeout;
	this->repeat  = repeat;

	FakeTimerManager::StartTimer(
	  this->timerId, timeout, repeat, [this]() { this->listener->OnTimer(this); });
}

void Timer::Stop()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	FakeTimerManager::StopTimer(this->timerId);
}

bool Timer::IsActive() const
{
	return FakeTimerManager::IsActive(this->timerId);
}

void Timer::Reset()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	if (!this->IsActive())
	{
		return;
	}

	if (this->repeat == 0u)
	{
		return;
	}

	this->Start(this->repeat, this->repeat);
}

void Timer::Restart()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	this->Stop();
	this->Start(this->timeout, this->repeat);
}

#else // FAKE_TIMERS

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

	this->uvHandle       = new uv_timer_t;
	this->uvHandle->data = static_cast<void*>(this);

	const int err = uv_timer_init(DepLibUV::GetLoop(), this->uvHandle);

	if (err != 0)
	{
		delete this->uvHandle;
		this->uvHandle = nullptr;

		MS_THROW_ERROR("uv_timer_init() failed: %s", uv_strerror(err));
	}
}

Timer::~Timer()
{
	MS_TRACE();

	if (!this->closed)
	{
		Close();
	}
}

void Timer::Close()
{
	MS_TRACE();

	if (this->closed)
	{
		return;
	}

	this->closed = true;

	uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle), static_cast<uv_close_cb>(onClose));
}

void Timer::Start(uint64_t timeout, uint64_t repeat)
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	this->timeout = timeout;
	this->repeat  = repeat;

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		Stop();
	}

	const int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), timeout, repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

void Timer::Stop()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	const int err = uv_timer_stop(this->uvHandle);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_stop() failed: %s", uv_strerror(err));
	}
}

bool Timer::IsActive() const
{
	return uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0;
}

void Timer::Reset()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) == 0)
	{
		return;
	}

	if (this->repeat == 0u)
	{
		return;
	}

	const int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->repeat, this->repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

void Timer::Restart()
{
	MS_TRACE();

	if (this->closed)
	{
		MS_THROW_ERROR("closed");
	}

	if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0)
	{
		Stop();
	}

	const int err =
	  uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer), this->timeout, this->repeat);

	if (err != 0)
	{
		MS_THROW_ERROR("uv_timer_start() failed: %s", uv_strerror(err));
	}
}

inline void Timer::OnUvTimer()
{
	MS_TRACE();

	// Notify the listener.
	this->listener->OnTimer(this);
}

#endif // FAKE_TIMERS

uint64_t GetTimeMs()
{
#if FAKE_TIMERS
	return static_cast<uint64_t>(FakeTimerManager::GetTimeMs());
#else
	return uv_hrtime() / 1000000u;
#endif
}

int64_t GetTimeMsInt64()
{
	return static_cast<int64_t>(GetTimeMs());
}

uint64_t GetTimeUs()
{
#if FAKE_TIMERS
	return FakeTimerManager::GetTimeMs() * 1000;
#else
	return uv_hrtime() / 1000u;
#endif
}

int64_t GetTimeUsInt64()
{
	return static_cast<int64_t>(GetTimeUs());
}
