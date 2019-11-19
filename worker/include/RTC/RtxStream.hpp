#ifndef MS_RTC_RTX_STREAM_HPP
#define MS_RTC_RTX_STREAM_HPP

#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtxStream.hpp"
#include <json.hpp>
#include <string>

using json = nlohmann::json;

namespace RTC
{
	class RtxStream
	{
	public:
		struct Params
		{
			void FillJson(json& jsonObject) const;

			uint32_t ssrc{ 0 };
			uint8_t payloadType{ 0 };
			RTC::RtpCodecMimeType mimeType;
			uint32_t clockRate{ 0 };
			std::string rrid;
			std::string cname;
		};

	public:
		explicit RtxStream(RTC::RtxStream::Params& params);
		virtual ~RtxStream();

		void FillJson(json& jsonObject) const;
		uint32_t GetSsrc() const;
		uint8_t GetPayloadType() const;
		const RTC::RtpCodecMimeType& GetMimeType() const;
		uint32_t GetClockRate() const;
		const std::string& GetRrid() const;
		const std::string& GetCname() const;
		uint8_t GetFractionLost() const;
		float GetLossPercentage() const;
		bool ReceivePacket(RTC::RtpPacket* packet);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);

	protected:
		bool UpdateSeq(RTC::RtpPacket* packet);
		uint32_t GetExpectedPackets() const;

	private:
		void InitSeq(uint16_t seq);

	protected:
		// Given as argument.
		Params params;
		// Others.
		//   https://tools.ietf.org/html/rfc3550#appendix-A.1 stuff.
		uint16_t maxSeq{ 0 };      // Highest seq. number seen.
		uint32_t cycles{ 0 };      // Shifted count of seq. number cycles.
		uint32_t baseSeq{ 0 };     // Base seq number.
		uint32_t badSeq{ 0 };      // Last 'bad' seq number + 1.
		uint32_t maxPacketTs{ 0 }; // Highest timestamp seen.
		uint64_t maxPacketMs{ 0 }; // When the packet with highest timestammp was seen.
		uint32_t packetsLost{ 0 };
		uint8_t fractionLost{ 0 };
		size_t packetsDiscarded{ 0 };
		size_t packetsCount{ 0 };

	private:
		// Whether at least a RTP packet has been received.
		bool started{ false };
		// Fields for generating Receiver Reports.
		uint32_t expectedPrior{ 0 };
		uint32_t receivedPrior{ 0 };
		uint32_t lastSrTimestamp{ 0 };
		uint64_t lastSrReceived{ 0 };
		uint32_t reportedPacketLost{ 0 };
	};

	/* Inline instance methods. */

	inline uint32_t RtxStream::GetSsrc() const
	{
		return this->params.ssrc;
	}

	inline uint8_t RtxStream::GetPayloadType() const
	{
		return this->params.payloadType;
	}

	inline const RTC::RtpCodecMimeType& RtxStream::GetMimeType() const
	{
		return this->params.mimeType;
	}

	inline uint32_t RtxStream::GetClockRate() const
	{
		return this->params.clockRate;
	}

	inline const std::string& RtxStream::GetRrid() const
	{
		return this->params.rrid;
	}

	inline const std::string& RtxStream::GetCname() const
	{
		return this->params.cname;
	}

	inline uint8_t RtxStream::GetFractionLost() const
	{
		return this->fractionLost;
	}

	inline float RtxStream::GetLossPercentage() const
	{
		return static_cast<float>(this->fractionLost) * 100 / 256;
	}

	inline uint32_t RtxStream::GetExpectedPackets() const
	{
		return (this->cycles + this->maxSeq) - this->baseSeq + 1;
	}
} // namespace RTC

#endif
