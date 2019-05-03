#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "Utils.hpp"
#include "RTC/RtpDataCounter.hpp"
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
		struct BufferItem
		{
			RTC::RtpPacket* packet{ nullptr };
			uint64_t resentAtTime{ 0 }; // Last time this packet was resent.
			uint8_t sentTimes{ 0 };     // Number of times this packet was resent.
			bool rtxEncoded{ false };   // Whether the packet has already been RTX encoded.
		};

	private:
		struct StorageItem
		{
			// Allow some more space for RTX encoding.
			uint8_t store[RTC::MtuSize + 200];
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
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t now);
		RTC::RTCP::SdesChunk* GetRtcpSdesChunk();
		void Pause() override;
		void Resume() override;
		uint32_t GetBitrate(uint64_t now) override;
		uint32_t GetBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer) override;
		uint32_t GetLayerBitrate(uint64_t now, uint8_t spatialLayer, uint8_t temporalLayer) override;

	private:
		void StorePacket(RTC::RtpPacket* packet);
		void ResetBufferItem(BufferItem& bufferItem);
		void UpdateBufferStartIdx();
		void ClearRetransmissionBuffer();
		void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);
		void UpdateScore(RTC::RTCP::ReceiverReport* report);

	private:
		uint32_t lostPrior{ 0 }; // Packets lost at last interval.
		uint32_t sentPrior{ 0 }; // Packets sent at last interval.
		std::vector<BufferItem> buffer;
		uint16_t bufferStartIdx{ 0 };
		size_t bufferSize{ 0 };
		std::vector<StorageItem> storage;
		float rtt{ 0 };
		uint16_t rtxSeq{ 0 };
		RTC::RtpDataCounter transmissionCounter;
	};

	/* Inline instance methods */

	inline void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
	{
		RTC::RtpStream::SetRtx(payloadType, ssrc);

		this->rtxSeq = Utils::Crypto::GetRandomUInt(0u, 0xFFFF);
	}

	inline uint32_t RtpStreamSend::GetBitrate(uint64_t now)
	{
		return this->transmissionCounter.GetBitrate(now);
	}

	inline void RtpStreamSend::ResetBufferItem(BufferItem& bufferItem)
	{
		delete bufferItem.packet;

		bufferItem.packet       = nullptr;
		bufferItem.resentAtTime = 0;
		bufferItem.sentTimes    = 0;
		bufferItem.rtxEncoded   = false;
	}
} // namespace RTC

#endif
