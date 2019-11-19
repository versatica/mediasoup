#ifndef MS_RTC_SHM_TRANSPORT_HPP
#define MS_RTC_SHM_TRANSPORT_HPP

#include "json.hpp"
#include "sfushm_av_media.h"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
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
		void SendRtpPacket(
		  RTC::RtpPacket* packet,
		  RTC::Consumer* consumer,
		  bool retransmitted = false,
		  bool probation     = false) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;

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

		// Handle shm writes
		std::string                  shm;      // stream file name

		// TODO: channels info downloaded from input data

		std::string                  logname;  // as copied from input data in ctor
		int                          loglevel;
		DepLibSfuShm::SfuShmMapItem *shmCtx;   // A handle to shm context so that to avoid lookup in DepLibSfuShm each time need to write smth
		sfushm_av_frame_frag_t       chunk;    // structure holding current chunk being written into shm, convenient to reuse timestamps data sometimes
	};
} // namespace RTC

#endif
