#ifndef MS_RTC_RTP_RECEIVER_H
#define MS_RTC_RTP_RECEIVER_H

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

	class RtpReceiver
	{
	public:
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void onRtpReceiverParameters(RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters) = 0;
			virtual void onRtpReceiverClosed(RtpReceiver* rtpReceiver) = 0;
		};

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void SetRtpListener(RTC::RtpListener* rtpListener);
		void SetRtcpListener(RTC::RtpListener* rtcpListener);
		RTC::RtpListener* GetRtpListener();
		RTC::RtpListener* GetRtcpListener();
		void RemoveRtpListener(RTC::RtpListener* rtpListener);

	public:
		// Passed by argument.
		uint32_t rtpReceiverId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::RtpListener* rtpListener = nullptr;
		RTC::RtpListener* rtcpListener = nullptr;
		// Allocated by this.
		RTC::RtpParameters* rtpParameters = nullptr;
	};

	/* Inline methods. */

	inline
	void RtpReceiver::SetRtpListener(RTC::RtpListener* rtpListener)
	{
		this->rtpListener = rtpListener;
	}

	inline
	void RtpReceiver::SetRtcpListener(RTC::RtpListener* rtcpListener)
	{
		this->rtcpListener = rtcpListener;
	}

	inline
	RTC::RtpListener* RtpReceiver::GetRtpListener()
	{
		return this->rtpListener;
	}

	inline
	RTC::RtpListener* RtpReceiver::GetRtcpListener()
	{
		return this->rtcpListener;
	}

	inline
	void RtpReceiver::RemoveRtpListener(RTC::RtpListener* rtpListener)
	{
		if (this->rtpListener == rtpListener)
			this->rtpListener = nullptr;

		if (this->rtcpListener == rtpListener)
			this->rtcpListener = nullptr;
	}
}

#endif
