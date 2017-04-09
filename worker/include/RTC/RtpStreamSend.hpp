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
			uint8_t store[65536];
		};

	private:
		struct BufferItem
		{
			uint32_t seq32         = 0; // RTP seq in 32 bytes plus 16 bits cycles.
			uint64_t resentAtTime  = 0;
			RTC::RtpPacket* packet = nullptr;
		};

	public:
		RtpStreamSend(RTC::RtpStream::Params& params, size_t bufferSize);
		virtual ~RtpStreamSend();

		virtual Json::Value toJson() const override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RequestRtpRetransmission(
		    uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t now);
		uint32_t GetRtt() const;

	private:
		void ClearBuffer();
		void StorePacket(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RtpStream. */
	protected:
		virtual void onInitSeq() override;

	private:
		std::vector<StorageItem> storage;
		typedef std::list<BufferItem> Buffer;
		Buffer buffer;

	private:
		size_t receivedBytes            = 0; // Bytes received.
		uint64_t lastPacketTimeMs       = 0; // Time (MS) when the last packet was received.
		uint32_t lastPacketRtpTimestamp = 0; // RTP Timestamp of the last packet.
		uint32_t rtt                    = 0; // Round trip time.
	};

	inline uint32_t RtpStreamSend::GetRtt() const
	{
		return this->rtt;
	}
}

#endif
