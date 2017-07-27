#ifndef MS_RTC_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_STREAM_SEND_HPP

#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpStream.hpp"
#include <list>
#include <vector>

namespace RTC
{
	class RtpStreamSend : public RtpStream
	{
	private:
		struct StorageItem
		{
			uint8_t store[RTC::MtuSize];
		};

	private:
		struct BufferItem
		{
			uint32_t seq32{ 0 }; // RTP seq in 32 bytes plus 16 bits cycles.
			uint64_t resentAtTime{ 0 };
			RTC::RtpPacket* packet{ nullptr };
		};

	public:
		RtpStreamSend(RTC::RtpStream::Params& params, size_t bufferSize);
		~RtpStreamSend() override;

		Json::Value ToJson() const override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RequestRtpRetransmission(
		    uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t now);
		uint32_t GetRtt() const;
		void SetRtx(uint8_t payloadType, uint32_t ssrc);
		bool HasRtx() const;
		void RtxEncode(RtpPacket* packet);

	private:
		void ClearBuffer();
		void StorePacket(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RtpStream. */
	protected:
		void OnInitSeq() override;

	private:
		std::vector<StorageItem> storage;
		using Buffer = std::list<BufferItem>;
		Buffer buffer;

	private:
		size_t receivedBytes{ 0 };            // Bytes received.
		uint64_t lastPacketTimeMs{ 0 };       // Time (MS) when the last packet was received.
		uint32_t lastPacketRtpTimestamp{ 0 }; // RTP Timestamp of the last packet.
		uint32_t rtt{ 0 };                    // Round trip time.

		// RTX related.
		bool hasRtx { false };
		uint8_t rtxPayloadType{ 0 };
		uint32_t rtxSsrc{ 0 };
		uint16_t rtxSeq{ 0 };
	};

	inline uint32_t RtpStreamSend::GetRtt() const
	{
		return this->rtt;
	}

	inline bool RtpStreamSend::HasRtx() const
	{
		return this->hasRtx;
	}
} // namespace RTC

#endif
