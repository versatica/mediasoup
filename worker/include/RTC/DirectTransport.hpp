#ifndef MS_RTC_DIRECT_TRANSPORT_HPP
#define MS_RTC_DIRECT_TRANSPORT_HPP

#include "RTC/Transport.hpp"

namespace RTC
{
	class DirectTransport : public RTC::Transport
	{
	public:
		DirectTransport(const std::string& id, RTC::Transport::Listener* listener, json& data);
		~DirectTransport() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) override;
		void HandleRequest(Channel::ChannelRequest* request) override;
		void HandleNotification(PayloadChannel::Notification* notification) override;

	private:
		bool IsConnected() const override;
		void SendRtpPacket(
		  RTC::Consumer* consumer,
		  RTC::RtpPacket* packet,
		  RTC::Transport::onSendCallback* cb = nullptr) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void SendMessage(
		  RTC::DataConsumer* dataConsumer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len,
		  onQueuedCallback* cb = nullptr) override;
		void SendSctpData(const uint8_t* data, size_t len) override;
		void RecvStreamClosed(uint32_t ssrc) override;
		void SendStreamClosed(uint32_t ssrc) override;
	};
} // namespace RTC

#endif
