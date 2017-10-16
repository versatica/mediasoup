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
	public:
		class Listener
		{
		public:
			virtual void OnRtpStreamHealthy(RTC::RtpStream* rtpStream)   = 0;
			virtual void OnRtpStreamUnhealthy(RTC::RtpStream* rtpStream) = 0;
		};

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

	public:
		RtpStreamSend(Listener* listener, RTC::RtpStream::Params& params, size_t bufferSize);
		~RtpStreamSend() override;

		Json::Value GetStats() override;
		bool ReceivePacket(RTC::RtpPacket* packet) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		void RequestRtpRetransmission(
		  uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container);
		RTC::RTCP::SenderReport* GetRtcpSenderReport(uint64_t now);
		void SetRtx(uint8_t payloadType, uint32_t ssrc);
		bool HasRtx() const;
		void RtxEncode(RtpPacket* packet);
		void ClearRetransmissionBuffer();
		bool IsHealthy() const;

	private:
		void StorePacket(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RtpStream. */
	protected:
		void CheckStatus() override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		std::vector<StorageItem> storage;
		using Buffer = std::list<BufferItem>;
		Buffer buffer;
		// Stats.
		uint32_t rtt{ 0 };
		// Others.
		bool healthy{ true };

	private:
		// Retransmittion related.
		bool hasRtx{ false };
		uint8_t rtxPayloadType{ 0 };
		uint32_t rtxSsrc{ 0 };
		uint16_t rtxSeq{ 0 };
	};

	inline bool RtpStreamSend::HasRtx() const
	{
		return this->hasRtx;
	}

	inline bool RtpStreamSend::IsHealthy() const
	{
		return this->healthy;
	}
} // namespace RTC

#endif
