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

		// Increase the index.
		if (++this->latestIndex >= this->windowItems)
			this->latestIndex = 0;

		// Latest index overlaps with the oldest one, remove it.
		if (this->latestIndex == this->oldestIndex && this->oldestIndex != -1)
		{
			MS_WARN_TAG(
			  info,
			  "calculation buffer full, windowSize:%" PRIu64 " ms windowItems:%" PRIu16,
			  this->windowSize,
			  this->windowItems);

			BufferItem& oldestItem = buffer[this->oldestIndex];
			this->totalCount -= oldestItem.count;
			oldestItem.count = 0u;
			oldestItem.time  = 0u;
			if (++this->oldestIndex >= this->windowItems)
				this->oldestIndex = 0;
		}

		// Update the latest item.
		BufferItem& item = buffer[this->latestIndex];
		item.count += size;
		item.time        = nowMs;
		this->latestTime = nowMs;

		// Set the oldest item index and time, if not set.
		if (this->oldestIndex < 0)
		{
			this->oldestIndex = this->latestIndex;
			this->oldestTime  = nowMs;
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

		// No item set
		if (this->latestIndex < 0 || this->oldestIndex < 0)
			return;

		uint64_t newOldestTime = nowMs - this->windowSize;

		// Oldest item already removed.
		if (newOldestTime <= this->oldestTime)
			return;

		// A whole window size time has elapsed since last entry. Reset the buffer.
		if (newOldestTime > this->latestTime)
		{
			Reset(nowMs);

			return;
		}

		while (this->oldestTime < newOldestTime)
		{
			BufferItem& oldestItem = buffer[this->oldestIndex];
			this->totalCount -= oldestItem.count;
			oldestItem.count = 0u;
			oldestItem.time  = 0u;

			if (++this->oldestIndex >= this->windowItems)
				this->oldestIndex = 0;

			const BufferItem& newOldestItem = buffer[this->oldestIndex];
			this->oldestTime                = newOldestItem.time;
		}
	}

	void RtpDataCounter::Update(RTC::RtpPacket* packet)
	{
		uint64_t nowMs = DepLibUV::GetTimeMs();

		this->packets++;
		this->rate.Update(packet->GetSize(), nowMs);
	}
} // namespace RTC
