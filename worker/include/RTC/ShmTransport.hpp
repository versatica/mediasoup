#ifndef MS_RTC_SHM_TRANSPORT_HPP
#define MS_RTC_SHM_TRANSPORT_HPP

#include "json.hpp"
#include "DepLibSfuShm.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include <string>

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
		bool RecvStreamMeta(json& data) override;
		DepLibSfuShm::ShmCtx* ShmCtx() { return &this->shmCtx; }

	private:
		bool IsConnected() const override;
		void SendRtpPacket(RTC::RtpPacket* packet, RTC::Transport::onSendCallback* cb = nullptr) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendSctpData(const uint8_t* data, size_t len) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnSctpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

	private:
		// Allocated by this.
		// Others.
		ListenIp listenIp;	

		bool rtcpMux{ false };
		bool comedia{ false };
		bool multiSource{ false };

		DepLibSfuShm::ShmCtx shmCtx; // shm writer context, needed here to begin shm initialization and correctly report transport stats
	};
} // namespace RTC

#endif
