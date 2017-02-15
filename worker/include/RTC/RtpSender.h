#ifndef MS_RTC_RTP_SENDER_H
#define MS_RTC_RTP_SENDER_H

#include "common.h"
#include "RTC/Transport.h"
#include "RTC/RtpStreamSend.h"
#include "RTC/RtpDictionaries.h"
#include "RTC/RtpPacket.h"
#include "RTC/RtpDataCounter.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/Sdes.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RTCP/CompoundPacket.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <unordered_set>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class RtpSender
	{
	public:
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void onRtpSenderClosed(RtpSender* rtpSender) = 0;
		};

	private:
		// Container of RTP packets to retransmit.
		static std::vector<RTC::RtpPacket*> rtpRetransmissionContainer;

	public:
		RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind);
		virtual ~RtpSender();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities);
		void Send(RTC::RtpParameters* rtpParameters);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport();
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters();
		void SendRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void ReceiveRtcpSdesChunk(RTC::RTCP::SdesChunk* chunk);
		void GetRtcp(RTC::RTCP::CompoundPacket *packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		uint32_t GetTransmissionRate(uint64_t now);

	private:
		void RetransmitRtpPacket(RTC::RtpPacket* packet);

	public:
		// Passed by argument.
		uint32_t rtpSenderId;
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::Transport* transport = nullptr;
		RTC::RtpCapabilities* peerCapabilities = nullptr;
		// Allocated by this.
		RTC::RtpParameters* rtpParameters = nullptr;
		RTC::RtpStreamSend* rtpStream = nullptr;
		// Others.
		std::unordered_set<uint8_t> supportedPayloadTypes;
		// Whether this RtpSender is valid according to Peer capabilities.
		bool available = false;
		// Sender Report holding the RTP stats.
		std::unique_ptr<RTC::RTCP::SenderReport> senderReport;
		std::unique_ptr<RTC::RTCP::SdesChunk> sdesChunk;
		// RTP counters.
		RTC::RtpDataCounter transmitted;
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime = 0;
		uint16_t maxRtcpInterval;

		// TODO: keep track of retransmitted data too.
		//RTC::RtpDataCounter retransmitted;
	};

	/* Inline methods. */

	inline
	void RtpSender::SetTransport(RTC::Transport* transport)
	{
		this->transport = transport;
	}

	inline
	RTC::Transport* RtpSender::GetTransport()
	{
		return this->transport;
	}

	inline
	void RtpSender::RemoveTransport(RTC::Transport* transport)
	{
		if (this->transport == transport)
			this->transport = nullptr;
	}

	inline
	RTC::RtpParameters* RtpSender::GetParameters()
	{
		return this->rtpParameters;
	}

	inline
	void RtpSender::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		this->senderReport.reset(new RTC::RTCP::SenderReport(report));
		this->senderReport->Serialize();
	};

	inline
	void RtpSender::ReceiveRtcpSdesChunk(RTC::RTCP::SdesChunk* chunk)
	{
		this->sdesChunk.reset(new RTC::RTCP::SdesChunk(chunk));
		this->sdesChunk->Serialize();
	};

	inline
	uint32_t RtpSender::GetTransmissionRate(uint64_t now)
	{
		return this->transmitted.GetRate(now);
	};
}

#endif
