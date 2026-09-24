#define MS_CLASS "RTC::RateCalculator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RateCalculator.hpp"
#include "Logger.hpp"
#include <cmath>  // std::trunc()
#include <limits> // std::numeric_limits()

namespace RTC
{
	/* Static. */

	// Factor of the window size within which the latest sample still counts as
	// recent.
	static constexpr double RecentSampleMarginFactor{ 1.5 };

	/* Instance methods. */

	RateCalculator::RateCalculator(int64_t windowSizeMs, float scale, uint16_t windowItems)
	{
		MS_TRACE();

		// Clamp the given values so every derived value is safe to use.
		this->windowSizeMs = std::max<int64_t>(windowSizeMs, 1);

		const int64_t items = std::max<int64_t>(windowItems, 1);

		// Item granularity, rounded up so that `items` items always suffice to cover
		// the window.
		this->itemSizeMs = (this->windowSizeMs + items - 1) / items;

		// Number of items needed to cover the whole window, rounded up. It is never
		// higher than `items`, and it guarantees that in-window data can never
		// overrun the ring. The window it spans overshoots windowSizeMs by less than
		// one item, which is inherent to splitting the window into items.
		this->buffer.resize(
		  static_cast<size_t>((this->windowSizeMs + this->itemSizeMs - 1) / this->itemSizeMs));

		this->scale = static_cast<double>(scale);
	}

	void RateCalculator::Update(size_t size, int64_t nowMs)
	{
		MS_TRACE();

		// Whether the window still held data when this sample arrived, and the sample
		// before it is recent enough for both to be taken as the same stretch of
		// traffic.
		// NOTE: Must be told before sliding the window, since sliding it is what
		// expires that data.
		const bool lastSampleIsRecent =
		  this->totalSamples != 0 && this->lastSampleTimeMs.has_value() &&
		  this->lastSampleTimeMs.value() >
		    nowMs -
		      static_cast<int64_t>(RecentSampleMarginFactor * static_cast<double>(this->windowSizeMs));

		// Ignore data older than the window. Should never happen.
		if (!SlideWindow(nowMs))
		{
			MS_WARN_DEV("given nowMs is older than the current window, ignoring data");

			return;
		}

		// The very first sample starts the measured period, and so does one that
		// finds the window empty after long enough without traffic. A stream that
		// just sends less often than the window keeps the period it had, so that it
		// is measured over the window instead of restarting at every sample and
		// hence never being measured at all.
		if (!this->firstSampleTimeMs.has_value() || (this->totalSamples == 0 && !lastSampleIsRecent))
		{
			this->firstSampleTimeMs = nowMs;
		}

		Item& item = this->buffer[this->newestItemIndex];

		item.count += size;
		item.samples++;

		this->totalCount += size;
		this->totalSamples++;
		this->bytes += size;
		this->lastSampleTimeMs = nowMs;
	}

	std::optional<int64_t> RateCalculator::GetRate(int64_t nowMs)
	{
		MS_TRACE();

		// If every key matches, the memoized rate is still exact. `lastRate` is a
		// pure function of the state the keys cover, so the value is right, and no
		// expiration can be pending: SlideWindow() already ran for this very `nowMs`
		// and `newestItemStartTimeMs` only moves forward afterwards, while the
		// initial and post Reset() state has an empty ring anyway.
		if (
		  nowMs == this->lastTimeMs && this->totalCount == this->lastTotalCount &&
		  this->totalSamples == this->lastTotalSamples)
		{
			MS_DEBUG_DEV("nothing changed since the latest call, early return");

			return this->lastRate;
		}

		SlideWindow(nowMs);

		// NOTE: Must be read after SlideWindow(), which may have expired data.
		this->lastTotalCount   = this->totalCount;
		this->lastTotalSamples = this->totalSamples;
		this->lastTimeMs       = nowMs;
		this->lastRate         = std::nullopt;

		// Either no sample has ever been taken, or none of them is still within the
		// window.
		if (!this->firstSampleTimeMs.has_value() || this->totalSamples == 0)
		{
			return this->lastRate;
		}

		// Period the data within the window actually spans. The instant the first
		// sample was taken at already counts as a millisecond, hence the increment,
		// which is also what keeps this from ever being zero. And once the data
		// reaches back beyond the window, the period is the window itself.
		const int64_t periodMs =
		  std::clamp<int64_t>(nowMs - this->firstSampleTimeMs.value() + 1, 1, this->windowSizeMs);

		// A single millisecond gives no duration to divide by, and a single sample
		// says how much data was taken at an instant but nothing about how fast it
		// is flowing, so neither is a measurement until the window has filled.
		if (periodMs <= 1 || (this->totalSamples <= 1 && periodMs < this->windowSizeMs))
		{
			return this->lastRate;
		}

		const double rate = std::trunc(
		  ((static_cast<double>(this->totalCount) * this->scale) / static_cast<double>(periodMs)) + 0.5);

		// A rate that does not fit is no rate at all, which is better than the
		// garbage that converting it would give.
		// NOTE: The comparison is not a strict one because converting the maximum of
		// int64_t to double rounds it up, so a rate equal to that value is already
		// out of range.
		if (rate >= static_cast<double>(std::numeric_limits<int64_t>::max()))
		{
			return this->lastRate;
		}

		this->lastRate = static_cast<int64_t>(rate);

		return this->lastRate;
	}

	void RateCalculator::Reset()
	{
		MS_TRACE();

		std::ranges::fill(this->buffer, Item{});

		this->firstSampleTimeMs.reset();
		this->lastSampleTimeMs.reset();
		this->newestItemIndex       = 0;
		this->newestItemStartTimeMs = 0;
		this->totalCount            = 0;
		this->totalSamples          = 0;
		this->lastRate              = std::nullopt;
		this->lastTimeMs            = 0;
		this->lastTotalCount        = 0;
		this->lastTotalSamples      = 0;
	}

	/**
	 * Expires the items that no longer belong to the window ending at `nowMs`,
	 * and makes the item holding `nowMs` the newest one.
	 *
	 * Returns false if `nowMs` is so far in the past that it lies outside of the
	 * window, in which case nothing is modified.
	 */
	bool RateCalculator::SlideWindow(int64_t nowMs)
	{
		MS_TRACE();

		// Time elapsed since the newest item started, negative when `nowMs` lies in
		// the past.
		const int64_t elapsedMs = nowMs - this->newestItemStartTimeMs;

		// `nowMs` is older than the whole window.
		if (elapsedMs <= -this->windowSizeMs)
		{
			return false;
		}

		// `nowMs` belongs to the newest item, or to an already existing one still
		// within the window, so there is nothing to expire.
		if (elapsedMs < this->itemSizeMs)
		{
			return true;
		}

		// NOTE: Positive since `elapsedMs` is not lower than `itemSizeMs` here.
		const int64_t steps = elapsedMs / this->itemSizeMs;

		// A whole window elapsed since the newest item, so every item is gone.
		if (std::cmp_greater_equal(steps, this->buffer.size()))
		{
			MS_DEBUG_DEV("a whole window elapsed, resetting every item");

			// NOTE: totalSamples is the sum of the samples of every item, so a zero
			// total means that the ring is already empty.
			if (this->totalSamples != 0)
			{
				std::ranges::fill(this->buffer, Item{});

				this->totalCount   = 0;
				this->totalSamples = 0;
			}

			this->newestItemIndex       = 0;
			this->newestItemStartTimeMs = nowMs;

			return true;
		}

		// Walk the ring forward. Every item being passed holds the count of exactly
		// buffer.size() items ago, which is now out of the window.
		for (int64_t i{ 0 }; i < steps; ++i)
		{
			if (++this->newestItemIndex == this->buffer.size())
			{
				this->newestItemIndex = 0;
			}

			Item& item = this->buffer[this->newestItemIndex];

			this->totalCount -= item.count;
			this->totalSamples -= item.samples;

			item = Item{};
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
