#ifndef MS_RTC_SHM_TRANSPORT_HPP
#define MS_RTC_SHM_TRANSPORT_HPP

#include "json.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"

using json = nlohmann::json;

namespace RTC
{
	class ShmTransport : public RTC::Transport, public RTC::UdpSocket::Listener
	{
	private:
		struct ListenIp
		{
			std::string ip;
			std::string announcedIp;
		};

	public:
		ShmTransport(const std::string& id, RTC::Transport::Listener* listener, json& data);
		~ShmTransport() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) override;
		void HandleRequest(Channel::Request* request) override;
		bool RecvStreamMeta(json& data) const override;		

	private:
		bool IsConnected() const override;
		void SendRtpPacket(
		  RTC::RtpPacket* packet,
		  RTC::Consumer* consumer,
		  bool retransmitted = false,
		  bool probation     = false) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnSctpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::Transport. */
	private:
		void UserOnNewProducer(RTC::Producer* producer) override;
		void UserOnNewConsumer(RTC::Consumer* consumer) override;
		void UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb) override;
		void UserOnSendSctpData(const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

	private:
		// Allocated by this.
		// Others.
		ListenIp listenIp;

		bool rtcpMux{ false };
		bool comedia{ false };
		bool multiSource{ false };

		// shm context structures: TBD
		// xcodeshm - shm context used for writing
		// maybe some sort of configs, and logging structs, dunno

		int32_t startChunkSeqId { -1 }; //TODO: which int
	};
} // namespace RTC

#endif
