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

		// If the elapsed time from the last used item is greater than the
		// window granularity, increase the index.
		if (this->newestTimeIndex < 0 || nowMs - this->newestTime >= this->itemSize)
		{
			this->newestTimeIndex++;
			this->newestTime = nowMs;
			if (this->newestTimeIndex >= this->windowItems)
				this->newestTimeIndex = 0;

			// Newest index overlaps with the oldest one, remove it.
			if (this->newestTimeIndex == this->oldestTimeIndex && this->oldestTimeIndex != -1)
			{
				MS_WARN_TAG(
				  info,
				  "calculation buffer full, windowSize:%zu ms windowItems:%" PRIu16,
				  this->windowSize,
				  this->windowItems);

				BufferItem& oldestItem = buffer[this->oldestTimeIndex];
				this->totalCount -= oldestItem.count;
				oldestItem.count = 0u;
				oldestItem.time  = 0u;
				if (++this->oldestTimeIndex >= this->windowItems)
					this->oldestTimeIndex = 0;
			}

			// Set the newest item.
			BufferItem& item = buffer[this->newestTimeIndex];
			item.count       = size;
			item.time        = nowMs;
		}
		else
		{
			// Update the newest item.
			BufferItem& item = buffer[this->newestTimeIndex];
			item.count += size;
		}

		// Set the oldest item index and time, if not set.
		if (this->oldestTimeIndex < 0)
		{
			this->oldestTimeIndex = this->newestTimeIndex;
			this->oldestTime      = nowMs;
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

		// No item set.
		if (this->newestTimeIndex < 0 || this->oldestTimeIndex < 0)
			return;

		uint64_t newoldestTime = nowMs - this->windowSize;

		// Oldest item already removed.
		if (newoldestTime <= this->oldestTime)
			return;

		// A whole window size time has elapsed since last entry. Reset the buffer.
		if (newoldestTime > this->newestTime)
		{
			Reset(nowMs);

			return;
		}

		while (this->oldestTime < newoldestTime)
		{
			BufferItem& oldestItem = buffer[this->oldestTimeIndex];
			this->totalCount -= oldestItem.count;
			oldestItem.count = 0u;
			oldestItem.time  = 0u;

			if (++this->oldestTimeIndex >= this->windowItems)
				this->oldestTimeIndex = 0;

			const BufferItem& newOldestItem = buffer[this->oldestTimeIndex];
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
