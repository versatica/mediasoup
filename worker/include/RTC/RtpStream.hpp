#ifndef MS_RTC_RTP_STREAM_HPP
#define MS_RTC_RTP_STREAM_HPP

#include "common.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include <json/json.h>

namespace RTC
{
	class RtpStream
	{
	public:
		struct Params
		{
			Json::Value ToJson() const;

			uint32_t ssrc{ 0 };
			uint8_t payloadType{ 0 };
			RTC::RtpCodecMime mime;
			uint32_t clockRate{ 0 };
			bool useNack{ false };
			bool usePli{ false };
			uint8_t absSendTimeId{ 0 }; // 0 means no abs-send-time id.
		};

	public:
		explicit RtpStream(RTC::RtpStream::Params& params);
		virtual ~RtpStream();

		virtual Json::Value ToJson() const = 0;
		uint32_t GetSsrc();
		virtual bool ReceivePacket(RTC::RtpPacket* packet);

	private:
		void InitSeq(uint16_t seq);
		bool UpdateSeq(RTC::RtpPacket* packet);

		/* Pure virtual methods that must be implemented by the subclass. */
	protected:
		virtual void OnInitSeq() = 0;

	protected:
		// Given as argument.
		RtpStream::Params params;
		// Others.
		bool started{ false }; // Whether at least a RTP packet has been received.
		// https://tools.ietf.org/html/rfc3550#appendix-A.1 stuff.
		uint16_t maxSeq{ 0 };        // Highest seq. number seen.
		uint32_t cycles{ 0 };        // Shifted count of seq. number cycles.
		uint32_t baseSeq{ 0 };       // Base seq number.
		uint32_t badSeq{ 0 };        // Last 'bad' seq number + 1.
		uint32_t probation{ 0 };     // Seq. packets till source is valid.
		uint32_t received{ 0 };      // Packets received.
		uint32_t expectedPrior{ 0 }; // Packet expected at last interval.
		uint32_t receivedPrior{ 0 }; // Packet received at last interval.
		// Others.
		uint32_t maxTimestamp{ 0 }; // Highest timestamp seen.
	};

	/* Inline instance methods. */

	inline uint32_t RtpStream::GetSsrc()
	{
		return this->params.ssrc;
	}
} // namespace RTC

#endif
