#ifndef MS_RTC_RTP_SENDER_H
#define MS_RTC_RTP_SENDER_H

#include "common.h"
#include "RTC/Transport.h"
#include "RTC/RtpDictionaries.h"
#include "RTC/RtpPacket.h"
#include "RTC/RtcpPacket.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
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
		virtual ~RtpSender();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities);
		void Send(RTC::RtpParameters* rtpParameters);
		void SetTransport(RTC::Transport* transport);
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters();
		void SendRtpPacket(RTC::RtpPacket* packet);

	public:
		// Passed by argument.
		uint32_t rtpSenderId;
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::Transport* transport = nullptr;
		RTC::RtpParameters* rtpParameters = nullptr;
		RTC::RtpCapabilities* peerCapabilities = nullptr;
		// Whether this RtpSender is valid according to Peer capabilities.
		bool available = false;
	};

	/* Inline methods. */

	inline
	void RtpSender::SetTransport(RTC::Transport* transport)
	{
		this->transport = transport;
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
	void RtpSender::SendRtpPacket(RTC::RtpPacket* packet)
	{
		if (this->available && this->transport)
			this->transport->SendRtpPacket(packet);
	}
}

#endif
