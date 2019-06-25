#ifndef MS_RTC_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_STREAM_RECV_HPP

#include "RTC/NackGenerator.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/RtpStream.hpp"
#include "handles/Timer.hpp"
#include <vector>

namespace RTC
{
	class RtpStreamRecv : public RTC::RtpStream,
	                      public RTC::NackGenerator::Listener,
	                      public Timer::Listener
	{
	public:
		class Listener : public RTC::RtpStream::Listener
		{
		public:
			virtual void OnRtpStreamSendRtcpPacket(
			  RTC::RtpStreamRecv* rtpStream, RTC::RTCP::Packet* packet) = 0;
			virtual void OnRtpStreamNeedWorstRemoteFractionLost(
			  RTC::RtpStreamRecv* rtpStream, uint8_t& worstRemoteFractionLost) = 0;
		};

	public:
		class TransmissionCounter
		{
		public:
			TransmissionCounter(uint8_t spatialLayers, uint8_t temporalLayers);
			void Update(RTC::RtpPacket* packet);
			uint32_t GetBitrate(uint64_t now);
			uint32_t GetBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer);
			uint32_t GetSpatialLayerBitrate(uint64_t now, uint8_t spatialLayer);
			uint32_t GetLayerBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer);
			size_t GetPacketCount() const;
			size_t GetBytes() const;

		private:
			std::vector<std::vector<RTC::RtpDataCounter>> spatialLayerCounters;
		};

	public:
		RtpStreamRecv(RTC::RtpStreamRecv::Listener* listener, RTC::RtpStream::Params& params);
		~RtpStreamRecv();

		void FillJsonStats(json& jsonObject) override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		bool ReceiveRtxPacket(RTC::RtpPacket* packet);
		RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void ReceiveRtcpXrDelaySinceLastRr(RTC::RTCP::DelaySinceLastRr::SsrcInfo* ssrcInfo);
		void RequestKeyFrame();
		void Pause() override;
		void Resume() override;
		uint32_t GetBitrate(uint64_t now) override;
		uint32_t GetBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer) override;
		uint32_t GetSpatialLayerBitrate(uint64_t now, uint8_t spatialLayer) override;
		uint32_t GetLayerBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer) override;

	private:
		void CalculateJitter(uint32_t rtpTimestamp);
		void UpdateScore();

		/* Pure virtual methods inherited from Timer. */
	protected:
		void OnTimer(Timer* timer) override;

		/* Pure virtual methods inherited from RTC::NackGenerator. */
	protected:
		void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override;
		void OnNackGeneratorKeyFrameRequired() override;

	private:
		uint32_t receivedPrior{ 0 };   // Packets received at last interval.
		uint32_t lastSrTimestamp{ 0 }; // The middle 32 bits out of 64 in the NTP
		                               // timestamp received in the most recent
		                               // sender report.
		uint64_t lastSrReceived{ 0 };  // Wallclock time representing the most recent
		                               // sender report arrival.
		uint32_t transit{ 0 };         // Relative transit time for prev packet.
		uint32_t jitter{ 0 };
		float rtt{ 0 };
		uint8_t firSeqNumber{ 0 };
		uint32_t reportedPacketLost{ 0 };
		std::unique_ptr<RTC::NackGenerator> nackGenerator;
		Timer* inactivityCheckPeriodicTimer{ nullptr };
		bool inactive{ false };     // Stream is inactive.
		uint64_t lastPacketAt{ 0 }; // Time last valid packet arrived.
		TransmissionCounter transmissionCounter;
	};

	/* Inline instance methods */

	inline uint32_t RtpStreamRecv::GetBitrate(uint64_t now)
	{
		return this->transmissionCounter.GetBitrate(now);
	}

	inline uint32_t RtpStreamRecv::GetBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer)
	{
		return this->transmissionCounter.GetBitrate(now, spatialLayer, temporalLayer);
	}

	inline uint32_t RtpStreamRecv::GetSpatialLayerBitrate(uint64_t now, uint8_t spatialLayer)
	{
		return this->transmissionCounter.GetSpatialLayerBitrate(now, spatialLayer);
	}

	inline uint32_t RtpStreamRecv::GetLayerBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer)
	{
		return this->transmissionCounter.GetLayerBitrate(now, spatialLayer, temporalLayer);
	}
} // namespace RTC

#endif
