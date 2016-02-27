#ifndef MS_RTC_RTP_SENDER_H
#define MS_RTC_RTP_SENDER_H

#include "common.h"
#include "RTC/Transport.h"
#include "RTC/RtpParameters.h"
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
		RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId);
		virtual ~RtpSender();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void Send(RTC::RtpParameters* rtpParameters);
		void SetTransport(RTC::Transport* transport);
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetRtpParameters();
		void SendRtpPacket(RTC::RtpPacket* packet);

	public:
		// Passed by argument.
		uint32_t rtpSenderId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::Transport* transport = nullptr;
		// Externally allocated but handled by this.
		RTC::RtpParameters* rtpParameters = nullptr;
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
	RTC::RtpParameters* RtpSender::GetRtpParameters()
	{
		return this->rtpParameters;
	}

	inline
	void RtpSender::SendRtpPacket(RTC::RtpPacket* packet)
	{
		if (this->transport)
			this->transport->SendRtpPacket(packet);
	}
}

#endif
