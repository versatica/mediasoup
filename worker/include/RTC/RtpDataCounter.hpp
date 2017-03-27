#ifndef MS_RTC_RTP_DATA_COUNTER_HPP
#define MS_RTC_RTP_DATA_COUNTER_HPP

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
		static constexpr float kBpsScale = 8000.0f;
		static constexpr float kbpsScale = 1000.0f;
		static constexpr size_t defaultWindowSize = 1000;

	public:
		explicit RateCalculator(size_t windowSize = defaultWindowSize, float scale = kBpsScale);
		void Update(size_t count, uint64_t now);
		uint32_t GetRate(uint64_t now);
		void Reset();

	private:
		void RemoveOldData(uint64_t now);
		void Reset(uint64_t now);

	private:
		struct BufferItem
		{
			size_t count = 0;
		};

	private:
		std::unique_ptr<BufferItem[]> buffer;

		// Time (in milliseconds) for oldest item in the time window.
		uint64_t oldestTime = 0;
		// Index for the oldest item in the time window.
		uint32_t oldestIndex = 0;
		// Total count in the time window.
		size_t totalCount = 0;
		// Window Size (in milliseconds).
		size_t windowSize = defaultWindowSize;
		// Scale in which the rate is represented.
		const float scale = kBpsScale;
	};

	class RtpDataCounter
	{
	public:
		void Update(RTC::RtpPacket* packet);
		uint32_t GetRate(uint64_t now);
		size_t GetPacketCount() const;
		size_t GetBytes() const;

	private:
		RateCalculator rate;
		size_t packets = 0;
		size_t bytes = 0;
	};

	/* Inline instance methods. */

	inline
	RateCalculator::RateCalculator(size_t windowSize, float scale) :
		windowSize(windowSize),
		scale(scale)
	{
		uint64_t now = DepLibUV::GetTime();

		this->Reset(now);
	}

	inline
	void RateCalculator::Reset()
	{
		this->Reset(this->oldestTime);
	}

	inline
	void RateCalculator::Reset(uint64_t now)
	{
		this->buffer.reset(new BufferItem[windowSize]);
		this->totalCount = 0;
		this->oldestIndex = 0;
		this->oldestTime = now - this->windowSize;
	}

	inline
	uint32_t RtpDataCounter::GetRate(uint64_t now)
	{
		return this->rate.GetRate(now);
	}

	inline
	size_t RtpDataCounter::GetPacketCount() const
	{
		return this->packets;
	}

	inline
	size_t RtpDataCounter::GetBytes() const
	{
		return this->bytes;
	}
}

#endif
