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
	public:
		static constexpr size_t DefaultWindowSize{ 1000u };
		static constexpr float DefaultBpsScale{ 8000.0f };
		static constexpr uint16_t DefaultWindowItems{ 100u };

	public:
		explicit RateCalculator(
		  size_t windowSizeMs  = DefaultWindowSize,
		  float scale          = DefaultBpsScale,
		  uint16_t windowItems = DefaultWindowItems);

		void Update(size_t size, uint64_t nowMs);

		uint32_t GetRate(uint64_t nowMs);

		size_t GetBytes() const
		{
			return this->bytes;
		}

		void Reset();

	private:
		bool SlideWindow(uint64_t nowMs);

	private:
		// Window size (in milliseconds). Always >= 1.
		size_t windowSizeMs{ DefaultWindowSize };
		// Item size (in milliseconds). Always >= 1.
		size_t itemSizeMs{ 1u };
		// Precomputed `scale / windowSizeMs`.
		double rateScale{ 0.0 };
		// Ring of items, each one holding the count of the data within it. Never
		// empty, and always long enough to cover the whole window.
		std::vector<size_t> buffer;
		// Index of the newest item. Always < buffer.size().
		size_t newestItemIndex{ 0u };
		// Time (in milliseconds) at which the newest item starts.
		uint64_t newestItemStartTimeMs{ 0u };
		// Sum of the count of every item.
		size_t totalCount{ 0u };
		// Total bytes accounted for. Not affected by Reset().
		size_t bytes{ 0u };
		// Rate memoized by GetRate(), only valid while both `lastTimeMs` and
		// `lastTotalCount` below still match. `lastTotalCount` is the one that makes
		// any Update() changing the rate invalidate this implicitly, so that the hot
		// path needs no memoization store.
		// NOTE: No "not calculated yet" mark is needed, since the initial and post
		// Reset() state is a valid entry on its own: a zero rate for a zero count.
		uint32_t lastRate{ 0u };
		// Time of the latest GetRate() call. Prevents reusing `lastRate` once time
		// has moved on and there is data pending expiration.
		uint64_t lastTimeMs{ 0u };
		// Total count at the latest GetRate() call.
		size_t lastTotalCount{ 0u };
	};

	class RtpDataCounter
	{
	public:
		explicit RtpDataCounter(
		  SharedInterface* shared, bool ignorePaddingOnlyPackets, size_t windowSizeMs = 2500)
		  : shared(shared), ignorePaddingOnlyPackets(ignorePaddingOnlyPackets), rate(windowSizeMs)
		{
		}

	public:
		void Update(const RTC::RTP::Packet* packet);

		uint32_t GetBitrate(uint64_t nowMs)
		{
			return this->rate.GetRate(nowMs);
		}

		size_t GetPacketCount() const
		{
			return this->packets;
		}

		size_t GetBytes() const
		{
			return this->rate.GetBytes();
		}

	private:
		SharedInterface* shared{ nullptr };
		// Whether the size of padding only RTP packets should not be taken into
		// account.
		bool ignorePaddingOnlyPackets{ false };
		RateCalculator rate;
		size_t packets{ 0u };
	};
} // namespace RTC

#endif
