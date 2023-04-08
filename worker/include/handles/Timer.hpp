#ifndef MS_TIMER_HPP
#define MS_TIMER_HPP

#include "common.hpp"
#include <uv.h>

#ifdef FAKE_TIMERS

#include <map>

class FakeTimerManager
{
public:
	static uint16_t GetNexTimerId();
	static void StartTimer(uint16_t timerId, uint64_t timeout, uint64_t repeat, std::function<void()> cb);
	static void StopTimer(uint16_t timerId);
	static bool IsActive(uint16_t timerId);

	static int64_t GetTimeMs()
	{
		return now;
	}
	static int64_t NextTimerTime();
	static void RunPending(int64_t now);
	static void RunLoop(int64_t maxTime = -1);

private:
	struct Timer
	{
		uint64_t nextTime;
		uint64_t repeat;
		std::function<void()> cb;
	};

	static int64_t now;
	static uint16_t nextTimerId;
	static std::map<uint16_t, Timer> timers;
};
#endif

class Timer
{
public:
	class Listener
	{
	public:
		virtual ~Listener() = default;

	public:
		virtual void OnTimer(Timer* timer) = 0;
	};

public:
	explicit Timer(Listener* listener);
	Timer& operator=(const Timer&) = delete;
	Timer(const Timer&)            = delete;
	~Timer();

public:
	void Close();
	void Start(uint64_t timeout, uint64_t repeat = 0);
	void Stop();
	void Reset();
	void Restart();
	uint64_t GetTimeout() const
	{
		return this->timeout;
	}
	uint64_t GetRepeat() const
	{
		return this->repeat;
	}
	bool IsActive() const;

	/* Callbacks fired by UV events. */
public:
	void OnUvTimer();

private:
	// Passed by argument.
	Listener* listener{ nullptr };
#ifdef FAKE_TIMERS
	uint64_t timerId;
#else
	// Allocated by this.
	uv_timer_t* uvHandle{ nullptr };
#endif
	// Others.
	bool closed{ false };
	uint64_t timeout{ 0u };
	uint64_t repeat{ 0u };
};

uint64_t GetTimeMs();
int64_t GetTimeMsInt64();
uint64_t GetTimeUs();
int64_t GetTimeUsInt64();

#endif
