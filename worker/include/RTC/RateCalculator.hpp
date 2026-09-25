#ifndef MS_RTC_RATE_CALCULATOR_HPP
#define MS_RTC_RATE_CALCULATOR_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include "SharedInterface.hpp"
#include <vector>

namespace RTC
{
	/**
	 * Sliding window rate meter.
	 *
	 * Data is accumulated into a ring of fixed duration items that covers the
	 * whole window. The in-window total is kept incrementally, so both Update()
	 * and GetRate() are O(1) amortized.
	 *
	 * It is considered that the time source increases monotonically. Timestamps
	 * going backwards are however tolerated (time comparisons are wrap safe):
	 * data still within the window is added to the newest item, older data is
	 * ignored, and nothing is ever expired ahead of time.
	 */
	class RateCalculator
	{
	private:
		struct Item
		{
			/**
			 * Sum of the sizes given to Update() within this item.
			 */
			uint64_t count{ 0 };
			/**
			 * Number of Update() calls accounted for within this item.
			 */
			uint64_t samples{ 0 };
		};

	public:
		static constexpr int64_t DefaultWindowSizeMs{ 1000 };
		static constexpr float DefaultBpsScale{ 8000.0f };
		static constexpr uint16_t DefaultWindowItems{ 100 };

	public:
		explicit RateCalculator(
		  int64_t windowSizeMs = DefaultWindowSizeMs,
		  float scale          = DefaultBpsScale,
		  uint16_t windowItems = DefaultWindowItems);

		/**
		 * Accounts for `size` units of data taken at `nowMs`.
		 *
		 * @remarks
		 * - A sample that arrives when the window holds none starts a new measured
		 *   period, unless the previous sample is recent enough for both to be taken
		 *   as the same stretch of traffic. That is what keeps a stream which sends
		 *   less often than the window from restarting its period at every sample and
		 *   hence never being measured at all.
		 */
		void Update(size_t size, int64_t nowMs);

		/**
		 * Rate of the data within the window ending at `nowMs`, computed over the
		 * period the data actually spans rather than over the whole window, so that
		 * a window which has not filled yet is not reported as a fraction of the
		 * rate it is measuring.
		 *
		 * While that period starts at a sample rather than at the edge of the window,
		 * the data of that sample is left out, since the period does not cover the
		 * time it took to arrive.
		 *
		 * Returns no value when there is nothing to measure, which is any of:
		 *
		 * - Not a single sample within the window.
		 * - A period of a single millisecond, which gives no duration to divide by.
		 * - A single sample while the window has not filled, since one sample says
		 *   how much data arrived at an instant but nothing about how fast it is
		 *   arriving.
		 *
		 * Returns zero when there are enough samples to measure but the data they
		 * add up to rounds down to zero at the configured scale, which takes
		 * Update() calls of size zero.
		 */
		std::optional<int64_t> GetRate(int64_t nowMs);

		uint64_t GetBytes() const
		{
			return this->bytes;
		}

		void Reset();

	private:
		bool SlideWindow(int64_t nowMs);

	private:
		// Window size (in milliseconds). Always >= 1.
		int64_t windowSizeMs{ DefaultWindowSizeMs };
		// Item size (in milliseconds). Always >= 1.
		int64_t itemSizeMs{ 1 };
		// Factor the in-window count is multiplied by before being divided by the
		// period it spans.
		double scale{ 0.0 };
		// How long after a sample the next one still counts as the same stretch of
		// traffic (in milliseconds). Derived from the window size.
		int64_t recentSampleMarginMs{ 0 };
		// Ring of items. Never empty, and always long enough to cover the whole
		// window.
		std::vector<Item> buffer;
		// Index of the newest item. Always < buffer.size().
		size_t newestItemIndex{ 0 };
		// Time (in milliseconds) at which the newest item starts.
		int64_t newestItemStartTimeMs{ 0 };
		// Time (in milliseconds) at which the measured period starts. Unset until the
		// very first sample, and from then on only ever moved forward by a sample
		// that finds the window empty after long enough without traffic.
		// NOTE: Expiration does not move it, so once the data reaches back beyond the
		// window it names a sample that is already gone. That is on purpose: from
		// that point on the period is the window itself, which is what the reader
		// clamps it to.
		std::optional<int64_t> firstSampleTimeMs;
		// Count of the sample that starts the measured period, which is left out of
		// the rate for as long as the period is anchored to it.
		uint64_t firstSampleCount{ 0 };
		// Time (in milliseconds) of the latest sample, which together with the window
		// still holding it is what tells a stream that simply sends less often than
		// the window apart from one that stopped.
		std::optional<int64_t> lastSampleTimeMs;
		// Sum of the count of every item.
		uint64_t totalCount{ 0 };
		// Sum of the samples of every item.
		uint64_t totalSamples{ 0 };
		// Total bytes accounted for. Not affected by Reset().
		// NOTE: Nothing bounds this one, unlike the in-window total above, so it is
		// as wide as the stats field it ends up in.
		uint64_t bytes{ 0 };
		// Rate memoized by GetRate(), only valid while `lastTimeMs`,
		// `lastTotalCount` and `lastTotalSamples` below all still match.
		// `lastTotalSamples` is the one that makes any Update() invalidate this
		// implicitly, so that the hot path needs no memoization store.
		// NOTE: No "not calculated yet" mark is needed, since the initial and post
		// Reset() state is a valid entry on its own: no rate for no samples.
		std::optional<int64_t> lastRate;
		// Time of the latest GetRate() call. Prevents reusing `lastRate` once time
		// has moved on and there is data pending expiration.
		int64_t lastTimeMs{ 0 };
		// Total count at the latest GetRate() call.
		uint64_t lastTotalCount{ 0 };
		// Total samples at the latest GetRate() call.
		uint64_t lastTotalSamples{ 0 };
	};

	class RtpDataCounter
	{
	public:
		explicit RtpDataCounter(
		  SharedInterface* shared, bool ignorePaddingOnlyPackets, int64_t windowSizeMs = 2500)
		  : shared(shared), ignorePaddingOnlyPackets(ignorePaddingOnlyPackets), rate(windowSizeMs)
		{
		}

	public:
		void Update(const RTC::RTP::Packet* packet);

		/**
		 * Bitrate (bps) of the RTP traffic within the window ending at `nowMs`,
		 * measured over the period that traffic actually spans.
		 *
		 * @returns No value while there is nothing to measure, which is any of: no
		 *   packet within the window, a period of a single millisecond, and a single
		 *   packet while the window has not filled.
		 */
		std::optional<int64_t> GetBitrate(int64_t nowMs)
		{
			return this->rate.GetRate(nowMs);
		}

		uint64_t GetPacketCount() const
		{
			return this->packets;
		}

		uint64_t GetBytes() const
		{
			return this->rate.GetBytes();
		}

	private:
		SharedInterface* shared{ nullptr };
		// Whether the size of padding only RTP packets should not be taken into
		// account.
		bool ignorePaddingOnlyPackets{ false };
		RateCalculator rate;
		// Total packets accounted for, which nothing bounds either.
		uint64_t packets{ 0 };
	};
} // namespace RTC

#endif
