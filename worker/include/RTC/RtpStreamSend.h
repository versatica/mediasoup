#ifndef MS_RTC_RTP_STREAM_SEND_H
#define MS_RTC_RTP_STREAM_SEND_H

#include "RTC/RtpStream.h"
#include "RTC/RTCP/SenderReport.h"
#include <vector>
#include <list>

namespace RTC
{
	class RtpStreamSend :
		public RtpStream
	{
	private:
		struct StorageItem
		{
			uint8_t store[65536];
		};

	private:
		struct BufferItem
		{
			uint32_t        seq32 = 0; // RTP seq in 32 bytes plus 16 bits cycles.
			RTC::RtpPacket* packet = nullptr;
		};

	public:
		RtpStreamSend(uint32_t clockRate, size_t bufferSize);
		virtual ~RtpStreamSend();

		bool ReceivePacket(RTC::RtpPacket* packet);
		void RequestRtpRetransmission(uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t now);

	private:
		void ClearBuffer();
		void StorePacket(RTC::RtpPacket* packet);
		void Dump();

	private:
		std::vector<StorageItem> storage;
		typedef std::list<BufferItem> Buffer;
		Buffer buffer;

	private:
		size_t receivedBytes = 0; // Bytes received.
		uint64_t lastPacketTimeMs = 0; // Time (MS) when the last packet was received.
		uint32_t lastPacketRtpTimestamp = 0; // RTP Timestamp of the last packet.
	};
}

#endif
