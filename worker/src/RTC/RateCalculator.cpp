#define MS_CLASS "RTC::RateCalculator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RateCalculator.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cmath>   // std::trunc()
#include <cstring> // std::memset()

namespace RTC
{
	RateCalculator::RateCalculator(size_t windowSizeMs, float scale, uint16_t windowItems)
	  : windowSizeMs(windowSizeMs), scale(scale), windowItems(windowItems),
	    itemSizeMs(std::max(windowSizeMs / windowItems, size_t{ 1 }))
	{
		MS_TRACE();

		this->buffer.resize(windowItems);

		std::memset(
		  static_cast<void*>(std::addressof(this->buffer.front())),
		  0,
		  sizeof(BufferItem) * this->buffer.size());
	}

	void RateCalculator::Update(size_t size, uint64_t nowMs)
	{
		MS_TRACE();

		// Ignore too old data. Should never happen.
		if (this->oldestItemStartTime.has_value() && Utils::Number<uint64_t>::IsLowerThan(nowMs, *this->oldestItemStartTime))
		{
			MS_WARN_DEV("nowMs < this->oldestItemStartTime, should never happen");

			return;
		}

		// Increase bytes.
		this->bytes += size;

		RemoveOldData(nowMs);

		// If the elapsed time from the newest item start time is greater than the
		// item size (in milliseconds), increase the item index.
		if (
		  this->newestItemIndex < 0 || !this->newestItemStartTime.has_value() ||
		  Utils::Number<uint64_t>::IsHigherOrEqualThan(
		    nowMs - *this->newestItemStartTime, this->itemSizeMs))
		{
			this->newestItemIndex++;
			this->newestItemStartTime = nowMs;

			if (this->newestItemIndex >= this->windowItems)
			{
				MS_DEBUG_DEV("this->newestItemIndex >= this->windowItems, setting this->newestItemIndex = 0");

				this->newestItemIndex = 0;
			}

			// Advance oldestItemIndex if buffer is full.
			// NOTE: This avoids a crash:
			//   https://github.com/versatica/mediasoup/issues/1316
			if (this->newestItemIndex == this->oldestItemIndex && this->oldestItemIndex != -1)
			{
				if (++this->oldestItemIndex >= this->windowItems)
				{
					this->oldestItemIndex = 0;
				}
			}

			MS_ASSERT(
			  this->newestItemIndex != this->oldestItemIndex || this->oldestItemIndex == -1,
			  "newest index overlaps with the oldest one [newestItemIndex:%" PRId32
			  ", oldestItemIndex:%" PRId32 "]",
			  this->newestItemIndex,
			  this->oldestItemIndex);

			// Set the newest item.
			BufferItem& item = this->buffer[this->newestItemIndex];
			item.count       = size;
			item.time        = nowMs;
		}
		else
		{
			// Update the newest item.
			BufferItem& item = this->buffer[this->newestItemIndex];
			item.count += size;
		}

		// Set the oldest item index and time, if not set.
		if (this->oldestItemIndex < 0)
		{
			MS_DEBUG_DEV(
			  "this->oldestItemIndex < 0, setting this->oldestItemIndex and this->oldestItemStartTime");

			this->oldestItemIndex     = this->newestItemIndex;
			this->oldestItemStartTime = nowMs;
		}

		this->totalCount += size;

		// Reset lastRate and lastTime so GetRate() will calculate rate again even
		// if called with same now in the same loop iteration.
		this->lastRate = 0;
		this->lastTime = std::nullopt;
	}

	uint32_t RateCalculator::GetRate(uint64_t nowMs)
	{
		MS_TRACE();

		if (this->lastTime.has_value() && nowMs == *this->lastTime)
		{
			MS_DEBUG_DEV("nowMs == this->lastTime, early return");

			return this->lastRate;
		}

		RemoveOldData(nowMs);

		const float scale = this->scale / this->windowSizeMs;

		this->lastTime = nowMs;
		this->lastRate = static_cast<uint32_t>(std::trunc(this->totalCount * scale + 0.5f));

		return this->lastRate;
	}

	void RateCalculator::Reset()
	{
		MS_TRACE();

		std::memset(
		  static_cast<void*>(std::addressof(this->buffer.front())),
		  0,
		  sizeof(BufferItem) * this->buffer.size());

		this->newestItemStartTime = std::nullopt;
		this->newestItemIndex     = -1;
		this->oldestItemStartTime = std::nullopt;
		this->oldestItemIndex     = -1;
		this->totalCount          = 0u;
		this->lastRate            = 0u;
		this->lastTime            = std::nullopt;
	}

	void RateCalculator::RemoveOldData(uint64_t nowMs)
	{
		MS_TRACE();

		if (!this->oldestItemStartTime.has_value())
		{
			return;
		}

		// No item set.
		if (this->newestItemIndex < 0 || this->oldestItemIndex < 0)
		{
			return;
		}

		const uint64_t newOldestTime = nowMs - this->windowSizeMs;

		// Oldest item already removed.
		if (Utils::Number<uint64_t>::IsLowerThan(newOldestTime, *this->oldestItemStartTime))
		{
			return;
		}

		// A whole window size time has elapsed since last entry. Reset the buffer.
		if (
		  this->newestItemStartTime.has_value() &&
		  Utils::Number<uint64_t>::IsHigherOrEqualThan(newOldestTime, *this->newestItemStartTime))
		{
			MS_DEBUG_DEV("newOldestTime >= this->newestItemStartTime, resetting the buffer");

			Reset();

			return;
		}

		while (Utils::Number<uint64_t>::IsHigherOrEqualThan(newOldestTime, *this->oldestItemStartTime))
		{
			BufferItem& oldestItem = this->buffer[this->oldestItemIndex];
			this->totalCount -= oldestItem.count;
			oldestItem.count = 0u;
			oldestItem.time  = 0u;

			if (++this->oldestItemIndex >= this->windowItems)
			{
				this->oldestItemIndex = 0;
			}

			const BufferItem& newOldestItem = this->buffer[this->oldestItemIndex];
			this->oldestItemStartTime       = newOldestItem.time;
		}
	}

	void RtpDataCounter::Update(RTC::RtpPacket* packet)
	{
		const uint64_t nowMs = DepLibUV::GetTimeMs();

		this->packets++;

		if (!this->ignorePaddingOnlyPackets || packet->GetPayloadLength() > 0)
		{
			this->rate.Update(packet->GetSize(), nowMs);
		}
	}
} // namespace RTC
