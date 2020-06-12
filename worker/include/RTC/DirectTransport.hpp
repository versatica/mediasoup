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
		void HandleRequest(Channel::Request* request) override;
		void HandleNotification(PayloadChannel::Notification* notification) override;

	private:
		bool IsConnected() const override;
		void SendRtpPacket(RTC::RtpPacket* packet, RTC::Transport::onSendCallback* cb = nullptr) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void SendSctpData(const uint8_t* data, size_t len) override;
	};
} // namespace RTC

#endif
