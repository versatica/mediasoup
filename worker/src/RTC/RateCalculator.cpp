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
		if (nowMs < this->oldestTimeIndex)
			return;

		// Increase bytes.
		this->bytes += size;

		RemoveOldData(nowMs);

		// Increase the index.
		if (++this->newestTimeIndex >= this->windowItems)
			this->newestTimeIndex = 0;

		// Newest index overlaps with the oldest one, remove it.
		if (this->newestTimeIndex == this->oldestIndex && this->oldestIndex != -1)
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

		// Update the newest item.
		BufferItem& item = buffer[this->newestTimeIndex];
		item.count += size;
		item.time        = nowMs;
		this->newestTime = nowMs;

		// Set the oldest item index and time, if not set.
		if (this->oldestIndex < 0)
		{
			this->oldestIndex     = this->newestTimeIndex;
			this->oldestTimeIndex = nowMs;
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
		if (this->newestTimeIndex < 0 || this->oldestIndex < 0)
			return;

		uint64_t newoldestTimeIndex = nowMs - this->windowSize;

		// Oldest item already removed.
		if (newoldestTimeIndex <= this->oldestTimeIndex)
			return;

		// A whole window size time has elapsed since last entry. Reset the buffer.
		if (newoldestTimeIndex > this->newestTime)
		{
			Reset(nowMs);

			return;
		}

		while (this->oldestTimeIndex < newoldestTimeIndex)
		{
			BufferItem& oldestItem = buffer[this->oldestIndex];
			this->totalCount -= oldestItem.count;
			oldestItem.count = 0u;
			oldestItem.time  = 0u;

			if (++this->oldestIndex >= this->windowItems)
				this->oldestIndex = 0;

			const BufferItem& newOldestItem = buffer[this->oldestIndex];
			this->oldestTimeIndex           = newOldestItem.time;
		}
	}

	void RtpDataCounter::Update(RTC::RtpPacket* packet)
	{
		uint64_t nowMs = DepLibUV::GetTimeMs();

		this->packets++;
		this->rate.Update(packet->GetSize(), nowMs);
	}
} // namespace RTC
