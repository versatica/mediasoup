#define MS_CLASS "RTC::RateCalculator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RateCalculator.hpp"
#include "Logger.hpp"
#include <cmath> // std::trunc()

namespace RTC
{
	void RateCalculator::Update(size_t size, uint64_t nowMs)
	{
		MS_TRACE();

		// Ignore too old data. Should never happen.
		if (nowMs < this->oldestTime)
			return;

		// Increase bytes.
		this->bytes += size;

		RemoveOldData(nowMs);

		if (!buffer.empty() && buffer.back().timestamp == nowMs)
		{
			buffer.back().count += size;
		}
		else
		{
			buffer.push_back(BufferItem(nowMs, size));
		}
		this->totalCount += size;

		// Reset lastRate and lastTime so GetRate() will calculate rate again even
		// if called with same now in the same loop iteration.
		this->lastRate = 0;
		this->lastTime = 0;
	}

	uint32_t RateCalculator::GetRate(uint64_t nowMs)
	{
		MS_TRACE();

		if (nowMs == this->lastTime)
			return this->lastRate;

		RemoveOldData(nowMs);

		float scale = this->scale / this->windowSize;

		this->lastTime = nowMs;
		this->lastRate = static_cast<uint32_t>(std::trunc(this->totalCount * scale + 0.5f));

		return this->lastRate;
	}

	inline void RateCalculator::RemoveOldData(uint64_t nowMs)
	{
		MS_TRACE();

		uint64_t newOldestTime = nowMs - this->windowSize;

		// Should never happen.
		if (newOldestTime < this->oldestTime)
		{
			MS_ERROR(
			  "current time [%" PRIu64 "] is older than a previous [%" PRIu64 "]",
			  newOldestTime,
			  this->oldestTime);

			return;
		}

		// We are in the same time unit (ms) as the last entry.
		if (newOldestTime == this->oldestTime)
			return;

		// A whole window size time has elapsed since last entry. Reset the buffer.
		if (newOldestTime > this->oldestTime + this->windowSize)
		{
			Reset(nowMs);

			return;
		}
		
		while (!buffer.empty() && buffer.front().timestamp < newOldestTime)
		{
			this->totalCount -= buffer.front().count;
			this->buffer.pop_front();
		}

		this->oldestTime = newOldestTime;
	}

	void RtpDataCounter::Update(RTC::RtpPacket* packet)
	{
		uint64_t nowMs = DepLibUV::GetTimeMs();

		this->packets++;
		this->rate.Update(packet->GetSize(), nowMs);
	}
} // namespace RTC
