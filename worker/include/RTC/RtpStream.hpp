#ifndef MS_RTC_RTP_STREAM_HPP
#define MS_RTC_RTP_STREAM_HPP

#include "common.hpp"
#include "json.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsPli.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

namespace RTC
{
	class RtpStream
	{
	protected:
		class Listener
		{
		public:
			virtual void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score) = 0;
		};

	public:
		struct Params
		{
			void FillJson(json& jsonObject) const;

			uint32_t ssrc{ 0 };
			uint8_t payloadType{ 0 };
			RTC::RtpCodecMimeType mimeType;
			uint32_t clockRate{ 0 };
			std::string rid;
			std::string cname;
			uint32_t rtxSsrc{ 0 };
			uint8_t rtxPayloadType{ 0 };
			bool useNack{ false };
			bool usePli{ false };
			bool useFir{ false };
			bool useInBandFec{ false };
			bool useDtx{ false };
			uint8_t spatialLayers{ 0 };
			uint8_t temporalLayers{ 0 };
		};

	public:
		RtpStream(RTC::RtpStream::Listener* listener, RTC::RtpStream::Params& params, uint8_t initialScore);
		virtual ~RtpStream();

		void FillJson(json& jsonObject) const;
		virtual void FillJsonStats(json& jsonObject);
		uint32_t GetSsrc() const;
		uint8_t GetPayloadType() const;
		const RTC::RtpCodecMimeType& GetMimeType() const;
		uint32_t GetClockRate() const;
		const std::string& GetRid() const;
		const std::string& GetCname() const;
		bool HasRtx() const;
		virtual void SetRtx(uint8_t payloadType, uint32_t ssrc);
		uint32_t GetRtxSsrc() const;
		uint8_t GetRtxPayloadType() const;
		uint8_t GetSpatialLayers() const;
		uint8_t GetTemporalLayers() const;
		virtual bool ReceivePacket(RTC::RtpPacket* packet);
		virtual void Pause()  = 0;
		virtual void Resume() = 0;
		void ResetScore(uint8_t score, bool notify);
		uint32_t GetBitrate(uint64_t now);
		uint8_t GetFractionLost() const;
		float GetLossPercentage() const;
		uint64_t GetMaxPacketMs() const;
		uint8_t GetScore() const;

	protected:
		bool UpdateSeq(RTC::RtpPacket* packet);
		void UpdateScore(uint8_t score);
		void PacketRetransmitted(RTC::RtpPacket* packet);
		void PacketRepaired(RTC::RtpPacket* packet);
		uint32_t GetExpectedPackets();

	private:
		void InitSeq(uint16_t seq);

	protected:
		// Given as argument.
		RTC::RtpStream::Listener* listener{ nullptr };
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
		size_t packetsRetransmitted{ 0 };
		size_t packetsRepaired{ 0 };
		size_t nackCount{ 0 };
		size_t nackPacketCount{ 0 };
		size_t pliCount{ 0 };
		size_t firCount{ 0 };
		size_t repairedPrior{ 0 };      // Packets repaired at last interval.
		size_t retransmittedPrior{ 0 }; // Packets retransmitted at last interval.
		uint32_t expectedPrior{ 0 };    // Packets expected at last interval.
		RTC::RtpDataCounter transmissionCounter;

	private:
		// Score related.
		uint8_t score{ 0 };
		std::vector<uint8_t> scores;
		// RTP stream data information for score calculation.
		int32_t totalSourceLoss{ 0 };
		int32_t totalReportedLoss{ 0 };
		size_t totalSentPackets{ 0 };
		// Whether at least a RTP packet has been received.
		bool started{ false };
	}; // namespace RTC

	/* Inline instance methods. */

	inline uint32_t RtpStream::GetSsrc() const
	{
		return this->params.ssrc;
	}

	inline uint8_t RtpStream::GetPayloadType() const
	{
		return this->params.payloadType;
	}

	inline const RTC::RtpCodecMimeType& RtpStream::GetMimeType() const
	{
		return this->params.mimeType;
	}

	inline uint32_t RtpStream::GetClockRate() const
	{
		return this->params.clockRate;
	}

	inline const std::string& RtpStream::GetRid() const
	{
		return this->params.rid;
	}

	inline const std::string& RtpStream::GetCname() const
	{
		return this->params.cname;
	}

	inline bool RtpStream::HasRtx() const
	{
		return this->params.rtxSsrc != 0;
	}

	inline void RtpStream::SetRtx(uint8_t payloadType, uint32_t ssrc)
	{
		this->params.rtxPayloadType = payloadType;
		this->params.rtxSsrc        = ssrc;
	}

	inline uint32_t RtpStream::GetRtxSsrc() const
	{
		return this->params.rtxSsrc;
	}

	inline uint8_t RtpStream::GetRtxPayloadType() const
	{
		return this->params.rtxPayloadType;
	}

	inline uint8_t RtpStream::GetSpatialLayers() const
	{
		return this->params.spatialLayers;
	}

	inline uint8_t RtpStream::GetTemporalLayers() const
	{
		return this->params.temporalLayers;
	}

	inline uint32_t RtpStream::GetBitrate(uint64_t now)
	{
		return this->transmissionCounter.GetRate(now);
	}

	inline uint8_t RtpStream::GetFractionLost() const
	{
		return this->fractionLost;
	}

	inline float RtpStream::GetLossPercentage() const
	{
		return static_cast<float>(this->fractionLost) * 100 / 256;
	}

	inline uint64_t RtpStream::GetMaxPacketMs() const
	{
		return this->maxPacketMs;
	}

	inline uint32_t RtpStream::GetExpectedPackets()
	{
		return (this->cycles + this->maxSeq) - this->baseSeq + 1;
	}

	inline uint8_t RtpStream::GetScore() const
	{
		return this->score;
	}
} // namespace RTC

#endif
