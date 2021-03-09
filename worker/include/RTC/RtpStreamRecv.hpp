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
			TransmissionCounter(uint8_t spatialLayers, uint8_t temporalLayers, size_t windowSize);
			void Update(RTC::RtpPacket* packet);
			uint32_t GetBitrate(uint64_t nowMs);
			uint32_t GetBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer);
			uint32_t GetSpatialLayerBitrate(uint64_t nowMs, uint8_t spatialLayer);
			uint32_t GetLayerBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer);
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
		RTC::RTCP::ReceiverReport* GetRtxRtcpReceiverReport();
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void ReceiveRtxRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void ReceiveRtcpXrDelaySinceLastRr(RTC::RTCP::DelaySinceLastRr::SsrcInfo* ssrcInfo);
		void RequestKeyFrame();
		void Pause() override;
		void Resume() override;
		uint32_t GetBitrate(uint64_t nowMs) override
		{
			return this->transmissionCounter.GetBitrate(nowMs);
		}
		uint32_t GetBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override
		{
			return this->transmissionCounter.GetBitrate(nowMs, spatialLayer, temporalLayer);
		}
		uint32_t GetSpatialLayerBitrate(uint64_t nowMs, uint8_t spatialLayer) override
		{
			return this->transmissionCounter.GetSpatialLayerBitrate(nowMs, spatialLayer);
		}
		uint32_t GetLayerBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override
		{
			return this->transmissionCounter.GetLayerBitrate(nowMs, spatialLayer, temporalLayer);
		}

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
		uint32_t expectedPrior{ 0u };      // Packets expected at last interval.
		uint32_t expectedPriorScore{ 0u }; // Packets expected at last interval for score calculation.
		uint32_t receivedPrior{ 0u };      // Packets received at last interval.
		uint32_t receivedPriorScore{ 0u }; // Packets received at last interval for score calculation.
		uint32_t lastSrTimestamp{ 0u };    // The middle 32 bits out of 64 in the NTP
		                                   // timestamp received in the most recent
		                                   // sender report.
		uint64_t lastSrReceived{ 0u };     // Wallclock time representing the most recent
		                                   // sender report arrival.
		int32_t transit{ 0u };             // Relative transit time for prev packet.
		uint32_t jitter{ 0u };
		uint8_t firSeqNumber{ 0u };
		uint32_t reportedPacketLost{ 0u };
		std::unique_ptr<RTC::NackGenerator> nackGenerator;
		Timer* inactivityCheckPeriodicTimer{ nullptr };
		bool inactive{ false };
		TransmissionCounter transmissionCounter;      // Valid media + valid RTX.
		RTC::RtpDataCounter mediaTransmissionCounter; // Just valid media.
	};
} // namespace RTC

#endif
