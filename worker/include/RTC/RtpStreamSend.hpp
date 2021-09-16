#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "RTC/RateCalculator.hpp"
#include "RTC/RtpStream.hpp"
#include <deque>

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
			// Original packet.
			RTC::RtpPacket* originalPacket{ nullptr };
			// Correct SSRC since original packet will have original ssrc.
			uint32_t ssrc{ 0 };
			// Correct sequence number since original packet will have original sequence number.
			uint16_t sequenceNumber{ 0 };
			// Cloned packet.
			RTC::RtpPacket* clonedPacket{ nullptr };
			// Last time this packet was resent.
			uint64_t resentAtMs{ 0u };
			// Number of times this packet was resent.
			uint8_t sentTimes{ 0u };
			// Whether the packet has been already RTX encoded.
			bool rtxEncoded{ false };
		};

	private:
		// Special container that can store `StorageItem*` elements addressable by their `uint16_t`
		// sequence number, while only taking as little memory as necessary to store the range between
		// minimum and maximum sequence number instead all 65536 potential elements.
		class StorageItemBuffer
		{
		public:
			~StorageItemBuffer();

			StorageItem* Get(uint16_t seq);
			bool Insert(uint16_t seq, StorageItem* storageItem);
			bool Remove(uint16_t seq);
			void Clear();

		private:
			uint16_t startSeq{ 0 };
			std::deque<StorageItem*> buffer;
		};

	public:
		RtpStreamSend(
		  RTC::RtpStreamSend::Listener* listener,
		  RTC::RtpStream::Params& params,
		  std::string& mid,
		  bool useNack);
		~RtpStreamSend() override;

		void FillJsonStats(json& jsonObject) override;
		void SetRtx(uint8_t payloadType, uint32_t ssrc) override;
		bool ReceivePacket(RTC::RtpPacket* packet, RTC::RtpPacket** clonedPacket);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t nowMs);
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
		void StorePacket(RTC::RtpPacket* packet, RTC::RtpPacket** clonedPacket);
		void ClearBuffer();
		void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);
		void UpdateScore(RTC::RTCP::ReceiverReport* report);

	private:
		uint32_t lostPriorScore{ 0u }; // Packets lost at last interval for score calculation.
		uint32_t sentPriorScore{ 0u }; // Packets sent at last interval for score calculation.
		StorageItemBuffer storageItemBuffer;
		uint16_t bufferStartSeq{ 0u };
		std::string mid;
		bool useNack;
		uint32_t retransmissionBufferSize;
		bool firstPacket{ true };
		uint16_t rtxSeq{ 0u };
		RTC::RtpDataCounter transmissionCounter;
	};
} // namespace RTC

#endif
