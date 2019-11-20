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
		static constexpr size_t DefaultWindowSize{ 1000 };
		static constexpr float DefaultBpsScale{ 8000.0f };

	public:
		RateCalculator(size_t windowSize = DefaultWindowSize, float scale = DefaultBpsScale);
		void Update(size_t size, uint64_t nowMs);
		uint32_t GetRate(uint64_t nowMs);
		size_t GetBytes() const;
		void Reset();

	private:
		void RemoveOldData(uint64_t nowMs);
		void Reset(uint64_t nowMs);

	private:
		struct BufferItem
		{
			size_t count{ 0 };
		};

	private:
		// Window Size (in milliseconds).
		size_t windowSize{ DefaultWindowSize };
		// Scale in which the rate is represented.
		float scale{ DefaultBpsScale };
		// Buffer to keep data.
		std::unique_ptr<BufferItem[]> buffer;
		// Time (in milliseconds) for oldest item in the time window.
		uint64_t oldestTime{ 0 };
		// Index for the oldest item in the time window.
		uint32_t oldestIndex{ 0 };
		// Total count in the time window.
		size_t totalCount{ 0 };
		// Total bytes transmitted.
		size_t bytes{ 0 };
		// Last value calculated by GetRate().
		uint32_t lastRate{ 0 };
		// Last time GetRate() was called.
		uint64_t lastTime{ 0 };
	};

	/* Inline instance methods. */

	inline RateCalculator::RateCalculator(size_t windowSize, float scale)
	  : windowSize(windowSize), scale(scale)
	{
		Reset();
	}

	inline size_t RateCalculator::GetBytes() const
	{
		return this->bytes;
	}

	inline void RateCalculator::Reset()
	{
		uint64_t nowMs = DepLibUV::GetTimeMs();

		Reset(nowMs);
	}

	inline void RateCalculator::Reset(uint64_t nowMs)
	{
		this->buffer.reset(new BufferItem[this->windowSize]);
		this->oldestTime  = nowMs - this->windowSize;
		this->oldestIndex = 0;
		this->totalCount  = 0;
		this->lastRate    = 0;
		this->lastTime    = 0;
	}

	class RtpDataCounter
	{
	public:
		explicit RtpDataCounter(size_t windowSize = 2500);

	public:
		void Update(RTC::RtpPacket* packet);
		uint32_t GetBitrate(uint64_t nowMs);
		size_t GetPacketCount() const;
		size_t GetBytes() const;

	private:
		RateCalculator rate;
		size_t packets{ 0 };
	};

	/* Inline instance methods. */

	inline RtpDataCounter::RtpDataCounter(size_t windowSize) : rate(windowSize)
	{
	}

	inline uint32_t RtpDataCounter::GetBitrate(uint64_t nowMs)
	{
		return this->rate.GetRate(nowMs);
	}

	inline size_t RtpDataCounter::GetPacketCount() const
	{
		return this->packets;
	}

	inline size_t RtpDataCounter::GetBytes() const
	{
		return this->rate.GetBytes();
	}
} // namespace RTC

#endif
