#ifndef MS_RTC_SHM_TRANSPORT_HPP
#define MS_RTC_SHM_TRANSPORT_HPP

#include "json.hpp"
#include "sfushm_av_media.h"
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
		bool RecvStreamMeta(json& data) const override;		

	private:
		bool IsConnected() const override;
		bool IsFullyConnected() const;
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

		bool isTransportConnectedCalled{ false }; // to account for the fact that "shm writer initialized" is not the same as "RTC::Transport child object is connected"
		// Handle shm writes
		std::string                  shm;      // stream file name

		std::string                  logname;  // as copied from input data in ctor
		int                          loglevel;

		// ShmTransport is responsible to writing RTCP packets into shm and various "metadata stuff" (TBD)
		DepLibSfuShm::SfuShmMapItem *shmCtx;   // A handle to shm context so that to avoid lookup in DepLibSfuShm each time need to write smth
		sfushm_av_frame_frag_t       chunk;    // structure holding current chunk being written into shm, convenient to reuse timestamps data sometimes
	};
} // namespace RTC

#endif
