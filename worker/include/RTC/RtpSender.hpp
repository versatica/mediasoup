#ifndef MS_RTC_RTP_SENDER_HPP
#define MS_RTC_RTP_SENDER_HPP

#include "common.hpp"
#include "RTC/Transport.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "Channel/Request.hpp"
#include "Channel/Notifier.hpp"
#include <unordered_set>
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

	public:
		RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind);

	private:
		virtual ~RtpSender();

	public:
		void Destroy();
		Json::Value toJson() const;
		void HandleRequest(Channel::Request* request);
		void SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities);
		void Send(RTC::RtpParameters* rtpParameters);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport() const;
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters() const;
		bool GetActive() const;
		void SendRtpPacket(RTC::RtpPacket* packet);
		void GetRtcp(RTC::RTCP::CompoundPacket *packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		uint32_t GetTransmissionRate(uint64_t now);

	private:
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void EmitActiveChange() const;

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
		// Whether this RtpSender has been disabled by the app.
		bool disabled = false;
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime = 0;
		uint16_t maxRtcpInterval;
		// RTP counters.
		RTC::RtpDataCounter transmittedCounter;
		// TODO: keep track of retransmitted data too.
		// RTC::RtpDataCounter retransmitted;
	};

	/* Inline methods. */

	inline
	void RtpSender::SetTransport(RTC::Transport* transport)
	{
		bool wasActive = this->GetActive();

		this->transport = transport;

		if (wasActive != this->GetActive())
			EmitActiveChange();
	}

	inline
	RTC::Transport* RtpSender::GetTransport() const
	{
		return this->transport;
	}

	inline
	void RtpSender::RemoveTransport(RTC::Transport* transport)
	{
		bool wasActive = this->GetActive();

		if (this->transport == transport)
			this->transport = nullptr;

		if (wasActive != this->GetActive())
			EmitActiveChange();
	}

	inline
	RTC::RtpParameters* RtpSender::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline
	bool RtpSender::GetActive() const
	{
		return (this->available && this->transport && !this->disabled);
	}

	inline
	uint32_t RtpSender::GetTransmissionRate(uint64_t now)
	{
		return this->transmittedCounter.GetRate(now);
	}
}

#endif
