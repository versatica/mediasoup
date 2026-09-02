#define MS_CLASS "RTC::RateCalculator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RateCalculator.hpp"
#include "Logger.hpp"
#include <cmath>   // std::trunc()
#include <limits>  // std::numeric_limits()
#include <utility> // std::cmp_less()

namespace RTC
{
	RateCalculator::RateCalculator(size_t windowSizeMs, float scale, uint16_t windowItems)
	{
		MS_TRACE();

		// Clamp the given values so every derived value is safe to use.
		this->windowSizeMs = std::max(windowSizeMs, size_t{ 1u });

		const size_t items = std::max<size_t>(windowItems, 1u);

		// Item granularity, rounded up so that `items` items always suffice to cover
		// the window.
		this->itemSizeMs = std::max((this->windowSizeMs + items - 1) / items, size_t{ 1u });

		// Number of items needed to cover the whole window, rounded up. It is never
		// higher than `items`, and it guarantees that in-window data can never
		// overrun the ring. The window it spans overshoots windowSizeMs by less than
		// one item, which is inherent to splitting the window into items.
		this->buffer.resize((this->windowSizeMs + this->itemSizeMs - 1) / this->itemSizeMs);

		this->rateScale = static_cast<double>(scale) / static_cast<double>(this->windowSizeMs);
	}

	void RateCalculator::Update(size_t size, uint64_t nowMs)
	{
		MS_TRACE();

		// Ignore data older than the window. Should never happen.
		if (!SlideWindow(nowMs))
		{
			MS_WARN_DEV("given nowMs is older than the current window, ignoring data");

			return;
		}

		this->buffer[this->newestItemIndex] += size;
		this->totalCount += size;
		this->bytes += size;
	}

	uint32_t RateCalculator::GetRate(uint64_t nowMs)
	{
		MS_TRACE();

		// If both keys match, the memoized rate is still exact. `lastRate` is a pure
		// function of `totalCount`, so the value is right, and no expiration can be
		// pending: SlideWindow() already ran for this very `nowMs` and
		// `newestItemStartTimeMs` only moves forward afterwards, while the initial and
		// post Reset() state has an empty ring anyway.
		if (nowMs == this->lastTimeMs && this->totalCount == this->lastTotalCount)
		{
			MS_DEBUG_DEV("nothing changed since the latest call, early return");

			return this->lastRate;
		}

		SlideWindow(nowMs);

		const double rate = std::trunc((static_cast<double>(this->totalCount) * this->rateScale) + 0.5);

		// NOTE: Must be read after SlideWindow(), which may have expired data.
		this->lastTotalCount = this->totalCount;
		this->lastTimeMs     = nowMs;
		this->lastRate       = static_cast<uint32_t>(
		  std::min(rate, static_cast<double>(std::numeric_limits<uint32_t>::max())));

		return this->lastRate;
	}

	void RateCalculator::Reset()
	{
		MS_TRACE();

		std::ranges::fill(this->buffer, 0u);

		this->newestItemIndex       = 0u;
		this->newestItemStartTimeMs = 0u;
		this->totalCount            = 0u;
		this->lastRate              = 0u;
		this->lastTimeMs            = 0u;
		this->lastTotalCount        = 0u;
	}

	/**
	 * Expires the items that no longer belong to the window ending at `nowMs`,
	 * and makes the item holding `nowMs` the newest one.
	 *
	 * Returns false if `nowMs` is so far in the past that it lies outside of the
	 * window, in which case nothing is modified.
	 */
	bool RateCalculator::SlideWindow(uint64_t nowMs)
	{
		MS_TRACE();

		// Time elapsed since the newest item started. The subtraction is done in
		// unsigned arithmetic and then reinterpreted as signed, so it is wrap safe
		// and negative when `nowMs` lies in the past.
		const auto elapsedMs = static_cast<int64_t>(nowMs - this->newestItemStartTimeMs);

		// `nowMs` is older than the whole window.
		if (elapsedMs <= -static_cast<int64_t>(this->windowSizeMs))
		{
			return false;
		}

		// `nowMs` belongs to the newest item, or to an already existing one still
		// within the window, so there is nothing to expire.
		if (std::cmp_less(elapsedMs, this->itemSizeMs))
		{
			return true;
		}

		const uint64_t steps = static_cast<uint64_t>(elapsedMs) / this->itemSizeMs;

		// A whole window elapsed since the newest item, so every item is gone.
		if (steps >= this->buffer.size())
		{
			MS_DEBUG_DEV("a whole window elapsed, resetting every item");

			// NOTE: totalCount is the sum of every item, so a zero total means that
			// the ring is already zeroed.
			if (this->totalCount != 0u)
			{
				std::ranges::fill(this->buffer, 0u);

				this->totalCount = 0u;
			}

			this->newestItemIndex       = 0u;
			this->newestItemStartTimeMs = nowMs;

			return true;
		}

		// Walk the ring forward. Every item being passed holds the count of exactly
		// buffer.size() items ago, which is now out of the window.
		for (uint64_t i{ 0u }; i < steps; ++i)
		{
			if (++this->newestItemIndex == this->buffer.size())
			{
				this->newestItemIndex = 0u;
			}

			this->totalCount -= this->buffer[this->newestItemIndex];

			this->buffer[this->newestItemIndex] = 0u;
		}

		// Advance by whole items rather than jumping to `nowMs`. The window is
		// derived from item geometry (buffer.size() * itemSizeMs) instead of from per
		// item timestamps, so absorbing the `elapsedMs % itemSizeMs` remainder here
		// would stretch items past itemSizeMs, making the ring span more time than
		// windowSizeMs and hence over-report the rate.
		this->newestItemStartTimeMs += steps * this->itemSizeMs;

		return true;
	}

	void RtpDataCounter::Update(const RTC::RTP::Packet* packet)
	{
		MS_TRACE();

		this->packets++;

		if (!this->ignorePaddingOnlyPackets || packet->GetPayloadLength() > 0)
		{
			this->rate.Update(packet->GetLength(), this->shared->GetTimeMs());
		}
	}
} // namespace RTC
