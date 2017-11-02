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
		struct Params
		{
			Json::Value ToJson() const;

			uint32_t ssrc{ 0 };
			uint8_t payloadType{ 0 };
			RTC::RtpCodecMimeType mimeType;
			uint32_t clockRate{ 0 };
			bool useNack{ false };
			bool usePli{ false };
		};

	public:
		explicit RtpStream(RTC::RtpStream::Params& params);
		virtual ~RtpStream();

		virtual Json::Value ToJson();
		virtual Json::Value GetStats();
		virtual bool ReceivePacket(RTC::RtpPacket* packet);
		uint32_t GetRate(uint64_t now);
		uint32_t GetSsrc() const;
		uint8_t GetPayloadType() const;
		uint32_t GetClockRate() const;
		const RTC::RtpCodecMimeType& GetMimeType() const;
		float GetLossPercentage() const;
		uint64_t GetMaxPacketMs() const;
		const std::string& GetId() const;
		void RestartStatusCheckTimer();
		void StopStatusCheckTimer();

	protected:
		bool UpdateSeq(RTC::RtpPacket* packet);

	private:
		void InitSeq(uint16_t seq);

		/* Pure virtual methods that must be implemented by the subclass. */
	protected:
		virtual void CheckStatus() = 0;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Stats.
		uint32_t packetsLost{ 0 };
		uint8_t fractionLost{ 0 };
		size_t packetsDiscarded{ 0 };
		size_t packetsRepaired{ 0 };
		size_t firCount{ 0 };
		size_t pliCount{ 0 };
		size_t nackCount{ 0 };
		size_t sliCount{ 0 };

		RtpDataCounter transmissionCounter;
		RtpDataCounter retransmissionCounter;

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
		Timer* statusCheckTimer{ nullptr };
		bool notifyStatus{ true };

	private:
		std::string rtpStreamId{};
	};

	/* Inline instance methods. */

	inline uint32_t RtpStream::GetSsrc() const
	{
		return this->params.ssrc;
	}

	inline uint8_t RtpStream::GetPayloadType() const
	{
		return this->params.payloadType;
	}

	inline uint32_t RtpStream::GetClockRate() const
	{
		return this->params.clockRate;
	}

	inline const RTC::RtpCodecMimeType& RtpStream::GetMimeType() const
	{
		return this->params.mimeType;
	}

	inline float RtpStream::GetLossPercentage() const
	{
		return static_cast<float>(this->fractionLost) * 100 / 256;
	}

	inline uint64_t RtpStream::GetMaxPacketMs() const
	{
		return this->maxPacketMs;
	}

	inline const std::string& RtpStream::GetId() const
	{
		return this->rtpStreamId;
	}
} // namespace RTC

#endif
