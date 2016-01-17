#ifndef MS_RTC_RTP_RECEIVER_H
#define MS_RTC_RTP_RECEIVER_H

#include "common.h"
#include "RTC/RtpParameters.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class RtpReceiver
	{
	public:
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void onRtpReceiverClosed(RtpReceiver* rtpReceiver) = 0;
		};

		/**
		 * RTC::Transport is the RtpListener.
		 */
		class RtpListener
		{
		public:
			virtual void onRtpListenerParameters(RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters) = 0;
			virtual void onRtpReceiverClosed(RtpReceiver* rtpReceiver) = 0;
		};

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetRtpListener(RtpListener* rtpListener);
		void SetRtpListenerForRtcp(RtpListener* rtcpListener);
		void RemoveRtpListener(RtpListener* rtpListener);

	public:
		// Passed by argument.
		uint32_t rtpReceiverId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RtpListener* rtpListener = nullptr;
		RtpListener* rtcpListener = nullptr;
		// Allocated by this.
		RTC::RtpParameters* rtpParameters = nullptr;
	};

	/* Inline methods. */

	inline
	void RtpReceiver::SetRtpListener(RtpListener* rtpListener)
	{
		this->rtpListener = rtpListener;
	}

	inline
	void RtpReceiver::SetRtpListenerForRtcp(RtpListener* rtcpListener)
	{
		this->rtcpListener = rtcpListener;
	}

	inline
	void RtpReceiver::RemoveRtpListener(RtpListener* rtpListener)
	{
		if (this->rtpListener == rtpListener)
			this->rtpListener = nullptr;

		if (this->rtcpListener == rtpListener)
			this->rtcpListener = nullptr;
	}
}

#endif
