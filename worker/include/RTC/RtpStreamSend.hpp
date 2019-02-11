#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "Utils.hpp"
#include "RTC/RtpStream.hpp"
#include <list>
#include <vector>

namespace RTC
{
	class RtpStreamSend : public RTC::RtpStream
	{
	private:
		struct StorageItem
		{
			uint8_t store[RTC::MtuSize];
		};

	private:
		struct BufferItem
		{
			uint16_t seq{ 0 }; // RTP seq.
			uint64_t resentAtTime{ 0 };
			RTC::RtpPacket* packet{ nullptr };
		};

#ifdef MS_TEST
		// For test units.
	public:
		static std::vector<RTC::RtpPacket*>& GetRetransmissionContainer();
#endif

	public:
		RtpStreamSend(RTC::RtpStream::Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize);
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
		void RetransmitPacket(RTC::RtpPacket* packet);

	private:
		void StorePacket(RTC::RtpPacket* packet);
		void ClearRetransmissionBuffer();

#ifdef MS_TEST
		// Make it public during test units.
	public:
#endif
		void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);

	private:
		std::vector<StorageItem> storage;
		std::list<BufferItem> buffer;
		float rtt{ 0 };
		uint16_t rtxSeq{ 0 };
	};

	inline void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
	{
		RTC::RtpStream::SetRtx(payloadType, ssrc);

		this->rtxSeq = Utils::Crypto::GetRandomUInt(0u, 0xFFFF);
	}
} // namespace RTC

#endif
