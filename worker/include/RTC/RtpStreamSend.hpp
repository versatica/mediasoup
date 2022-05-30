#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "RTC/RateCalculator.hpp"
#include "RTC/RtpStream.hpp"
#include <vector>

namespace RTC
{
	class RtpStreamSend : public RTC::RtpStream
	{
	public:
		class Listener : public RTC::RtpStream::Listener
		{
		public:
			virtual void OnRtpStreamRetransmitRtpPacket(
			  RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) = 0;
		};

	public:
		struct StorageItem
		{
			// Cloned packet.
			RTC::RtpPacket* packet{ nullptr };
			// Memory to hold the cloned packet (with extra space for RTX encoding).
			uint8_t store[RTC::MtuSize + 100];
			// Last time this packet was resent.
			uint64_t resentAtMs{ 0u };
			// Number of times this packet was resent.
			uint8_t sentTimes{ 0u };
			// Whether the packet has been already RTX encoded.
			bool rtxEncoded{ false };
		};

	public:
		RtpStreamSend(
		  RTC::RtpStreamSend::Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize);
		~RtpStreamSend() override;

		void FillJsonStats(json& jsonObject) override;
		void SetRtx(uint8_t payloadType, uint32_t ssrc) override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void ReceiveRtcpXrReceiverReferenceTime(RTC::RTCP::ReceiverReferenceTime* report);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t nowMs);
		RTC::RTCP::DelaySinceLastRr::SsrcInfo* GetRtcpXrDelaySinceLastRr(uint64_t nowMs);
		RTC::RTCP::SdesChunk* GetRtcpSdesChunk();
		void Pause() override;
		void Resume() override;
		uint32_t GetBitrate(uint64_t nowMs) override
		{
			return this->transmissionCounter.GetBitrate(nowMs);
		}
		uint32_t GetBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;
		uint32_t GetSpatialLayerBitrate(uint64_t nowMs, uint8_t spatialLayer) override;
		uint32_t GetLayerBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;

	private:
		void StorePacket(RTC::RtpPacket* packet);
		void ClearBuffer();
		void UpdateBufferStartIdx();
		void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);
		void UpdateScore(RTC::RTCP::ReceiverReport* report);

	private:
		uint32_t lostPriorScore{ 0u }; // Packets lost at last interval for score calculation.
		uint32_t sentPriorScore{ 0u }; // Packets sent at last interval for score calculation.
		std::vector<StorageItem*> buffer;
		uint16_t bufferStartIdx{ 0u };
		size_t bufferSize{ 0u };
		std::vector<StorageItem> storage;
		uint16_t rtxSeq{ 0u };
		RTC::RtpDataCounter transmissionCounter;
		uint32_t lastRrTimestamp{ 0u };  // The middle 32 bits out of 64 in the NTP
		                                 // timestamp received in the most recent
		                                 // receiver reference timestamp.
		uint64_t lastRrReceivedMs{ 0u }; // Wallclock time representing the most recent
		                                 // receiver reference timestamp arrival.
	};
} // namespace RTC

#endif
