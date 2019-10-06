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

		// Set data in the index before the oldest index.
		uint32_t offset = this->windowSize - 1;
		uint32_t index  = this->oldestIndex + offset;

		if (index >= this->windowSize)
			index -= this->windowSize;

		this->buffer[index].count += size;
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

		while (this->oldestTime < newOldestTime)
		{
			const BufferItem& oldestItem = buffer[this->oldestIndex];

			this->totalCount -= oldestItem.count;
			this->buffer[this->oldestIndex] = BufferItem();

			if (++this->oldestIndex >= this->windowSize)
				this->oldestIndex = 0;

			++this->oldestTime;
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
