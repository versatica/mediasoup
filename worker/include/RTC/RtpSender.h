#ifndef MS_RTC_RTP_SENDER_H
#define MS_RTC_RTP_SENDER_H

#include "common.h"
#include "RTC/RtpParameters.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <json/json.h>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class RtpListener;

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
		void SetRtpListener(RTC::RtpListener* rtpListener);
		RTC::RtpListener* GetRtpListener();
		void RemoveRtpListener(RTC::RtpListener* rtpListener);
		RTC::RtpParameters* GetRtpParameters();
		void NotifyParameters();

	public:
		// Passed by argument.
		uint32_t rtpSenderId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::RtpListener* rtpListener = nullptr;
		// Externally allocated but handled by this.
		RTC::RtpParameters* rtpParameters = nullptr;
	};

	/* Inline methods. */

	inline
	void RtpSender::SetRtpListener(RTC::RtpListener* rtpListener)
	{
		this->rtpListener = rtpListener;
	}

	inline
	RTC::RtpListener* RtpSender::GetRtpListener()
	{
		return this->rtpListener;
	}

	inline
	void RtpSender::RemoveRtpListener(RTC::RtpListener* rtpListener)
	{
		if (this->rtpListener == rtpListener)
			this->rtpListener = nullptr;
	}

	inline
	RTC::RtpParameters* RtpSender::GetRtpParameters()
	{
		return this->rtpParameters;
	}
}

#endif
