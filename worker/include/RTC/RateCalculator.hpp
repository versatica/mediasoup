#ifndef MS_RTC_RATE_CALCULATOR_HPP
#define MS_RTC_RATE_CALCULATOR_HPP

#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	// It is considered that the time source increases monotonically.
	// ie: the current timestamp can never be minor than a timestamp in the past.
	class RateCalculator
	{
	public:
		static constexpr size_t DefaultWindowSize{ 1000u };
		static constexpr float DefaultBpsScale{ 8000.0f };
		static constexpr uint16_t DefaultWindowItems{ 1000u };

	public:
		RateCalculator(
		  size_t windowSize    = DefaultWindowSize,
		  float scale          = DefaultBpsScale,
		  uint16_t windowItems = DefaultWindowItems)
		  : windowSize(windowSize), scale(scale), windowItems(windowItems)
		{
			Reset();
		}
		void Update(size_t size, uint64_t nowMs);
		uint32_t GetRate(uint64_t nowMs);
		size_t GetBytes() const
		{
			return this->bytes;
		}
		void Reset()
		{
			uint64_t nowMs = DepLibUV::GetTimeMs();

			Reset(nowMs);
		}

	private:
		void RemoveOldData(uint64_t nowMs);
		void Reset(uint64_t nowMs)
		{
			this->buffer.reset(new BufferItem[this->windowItems]);
			this->newestTime      = 0u;
			this->newestTimeIndex = -1;
			this->oldestTimeIndex = 0u;
			this->oldestIndex     = -1;
			this->totalCount      = 0u;
			this->lastRate        = 0u;
			this->lastTime        = 0u;
		}

	private:
		struct BufferItem
		{
			size_t count{ 0u };
			uint64_t time{ 0u };
		};

	private:
		// Window Size (in milliseconds).
		size_t windowSize{ DefaultWindowSize };
		// Scale in which the rate is represented.
		float scale{ DefaultBpsScale };
		// Window Size (number of items).
		uint16_t windowItems{ DefaultWindowItems };
		// Buffer to keep data.
		std::unique_ptr<BufferItem[]> buffer;
		// Time (in milliseconds) for last item in the time window.
		uint64_t newestTime{ 0u };
		// Index for the last item in the time window.
		int32_t newestTimeIndex{ -1 };
		// Time (in milliseconds) for oldest item in the time window.
		uint64_t oldestTimeIndex{ 0u };
		// Index for the oldest item in the time window.
		int32_t oldestIndex{ -1 };
		// Total count in the time window.
		size_t totalCount{ 0u };
		// Total bytes transmitted.
		size_t bytes{ 0u };
		// Last value calculated by GetRate().
		uint32_t lastRate{ 0u };
		// Last time GetRate() was called.
		uint64_t lastTime{ 0u };
	};

	class RtpDataCounter
	{
	public:
		explicit RtpDataCounter(size_t windowSize = 2500) : rate(windowSize)
		{
		}

	public:
		void Update(RTC::RtpPacket* packet);
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
		RateCalculator rate;
		size_t packets{ 0u };
	};
} // namespace RTC

#endif
