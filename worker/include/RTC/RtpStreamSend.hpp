#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "RTC/RateCalculator.hpp"
#include "RTC/RtpStream.hpp"
#include "handles/Timer.hpp"
#include <deque>

namespace RTC
{
	class RtpStreamSend : public RTC::RtpStream, public Timer::Listener
	{
	public:
		// Minimum retransmission buffer size (ms).
		const static uint32_t MinRetransmissionDelayMs;
		// Maximum retransmission buffer size for video (ms).
		const static uint32_t MaxRetransmissionDelayForVideoMs;
		// Maximum retransmission buffer size for audio (ms).
		const static uint32_t MaxRetransmissionDelayForAudioMs;

	public:
		class Listener : public RTC::RtpStream::Listener
		{
		public:
			virtual void OnRtpStreamRetransmitRtpPacket(
			  RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) = 0;
		};

	public:
		struct RetransmissionItem
		{
			void Reset();

			// Original packet.
			std::shared_ptr<RTC::RtpPacket> packet{ nullptr };
			// Correct SSRC since original packet may not have the same.
			uint32_t ssrc{ 0u };
			// Correct sequence number since original packet may not have the same.
			uint16_t sequenceNumber{ 0u };
			// Correct timestamp since original packet may not have the same.
			uint32_t timestamp{ 0u };
			// System time when this packet was received..
			uint64_t receivedAtMs{ 0u };
			// Last time this packet was resent.
			uint64_t resentAtMs{ 0u };
			// Number of times this packet was resent.
			uint8_t sentTimes{ 0u };
		};

	private:
		// Special container that stores `RetransmissionItem`* elements addressable
		// by their `uint16_t` sequence number, while only taking as little memory
		// as necessary to store the range covering a maximum of
		// `MaxRetransmissionDelayForVideoMs` or `MaxRetransmissionDelayForAudioMs`
		// ms.
		class RetransmissionBuffer
		{
		public:
			explicit RetransmissionBuffer(size_t maxEntries);
			~RetransmissionBuffer();

			size_t GetSize() const;
			RetransmissionItem* GetOldest() const;
			RetransmissionItem* GetNewest() const;
			RetransmissionItem* Get(uint16_t seq) const;
			void RemoveOldest();
			void Clear();
			void Insert(uint16_t seq, RetransmissionItem* retransmissionItem, uint32_t retransmissionDelayMs);

		private:
			size_t maxEntries;
			uint16_t startSeq{ 0u };
			std::deque<RetransmissionItem*> buffer;
		};

	public:
		RtpStreamSend(
		  RTC::RtpStreamSend::Listener* listener, RTC::RtpStream::Params& params, std::string& mid);
		~RtpStreamSend() override;

		void FillJsonStats(json& jsonObject) override;
		void SetRtx(uint8_t payloadType, uint32_t ssrc) override;
		bool ReceivePacket(RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket);
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
		void StorePacket(RTC::RtpPacket* packet, std::shared_ptr<RTC::RtpPacket>& sharedPacket);
		void ClearOldStoredPackets();
		void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);
		void UpdateScore(RTC::RTCP::ReceiverReport* report);

		/* Pure virtual methods inherited from Timer. */
	protected:
		void OnTimer(Timer* timer) override;

	private:
		// Packets lost at last interval for score calculation.
		uint32_t lostPriorScore{ 0u };
		// Packets sent at last interval for score calculation.
		uint32_t sentPriorScore{ 0u };
		RetransmissionBuffer retransmissionBuffer;
		std::string mid;
		uint32_t maxRetransmissionDelayMs;
		uint32_t currentRetransmissionDelayMs;
		uint16_t rtxSeq{ 0u };
		RTC::RtpDataCounter transmissionCounter;
		// The middle 32 bits out of 64 in the NTP timestamp received in the most
		// recent receiver reference timestamp.
		uint32_t lastRrTimestamp{ 0u };
		// Wallclock time representing the most recent receiver reference timestamp
		// arrival.
		uint64_t lastRrReceivedMs{ 0u };
		Timer* clearBufferPeriodicTimer{ nullptr };
	};
} // namespace RTC

#endif
