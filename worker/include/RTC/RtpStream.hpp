#ifndef MS_RTC_RTP_STREAM_HPP
#define MS_RTC_RTP_STREAM_HPP

#include "common.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>

namespace RTC
{
	class RtpStream : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpStreamHealthReport(RTC::RtpStream* rtpStream, bool healthy) = 0;
		};

	public:
		struct Params
		{
			Json::Value ToJson() const;

			uint32_t ssrc{ 0 };
			uint8_t payloadType{ 0 };
			RTC::RtpCodecMimeType mime;
			uint32_t clockRate{ 0 };
			bool useNack{ false };
			bool usePli{ false };
		};

	public:
		explicit RtpStream(RTC::RtpStream::Params& params);
		virtual ~RtpStream();

		virtual Json::Value ToJson();
		virtual bool ReceivePacket(RTC::RtpPacket* packet);
		uint32_t GetBitRate();
		uint32_t GetSsrc();
		uint8_t GetPayloadType();
		const RTC::RtpCodecMimeType& GetMimeType() const;
		bool IsHealthy();

	protected:
		bool UpdateSeq(RTC::RtpPacket* packet);

	private:
		void InitSeq(uint16_t seq);

		/* Pure virtual methods that must be implemented by the subclass. */
	protected:
		virtual void CheckHealth() = 0;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	protected:
		// Given as argument.
		RtpStream::Params params;
		// Others.
		// Whether at least a RTP packet has been received.
		//   https://tools.ietf.org/html/rfc3550#appendix-A.1 stuff.
		bool started{ false };
		uint16_t maxSeq{ 0 };  // Highest seq. number seen.
		uint32_t cycles{ 0 };  // Shifted count of seq. number cycles.
		uint32_t baseSeq{ 0 }; // Base seq number.
		uint32_t badSeq{ 0 };  // Last 'bad' seq number + 1.
		// Others.
		uint32_t maxPacketTs{ 0 }; // Highest timestamp seen.
		uint64_t maxPacketMs{ 0 }; // When the packet with highest timestammp was seen.
		Timer* healthCheckTimer{ nullptr };
		bool healthy{ true };
		uint32_t totalLost{ 0 };
		uint8_t fractionLost{ 0 };
		uint32_t jitter{ 0 };
		uint32_t rtt{ 0 };
		// RTP counters.
		RTC::RtpDataCounter counter;
	};

	/* Inline instance methods. */

	inline uint32_t RtpStream::GetSsrc()
	{
		return this->params.ssrc;
	}

	inline uint8_t RtpStream::GetPayloadType()
	{
		return this->params.payloadType;
	}

	inline const RTC::RtpCodecMimeType& RtpStream::GetMimeType() const
	{
		return this->params.mime;
	}

	inline bool RtpStream::IsHealthy()
	{
		return this->healthy;
	}
} // namespace RTC

#endif
