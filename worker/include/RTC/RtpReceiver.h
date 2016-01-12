#ifndef MS_RTC_RTP_RECEIVER_H
#define MS_RTC_RTP_RECEIVER_H

#include "common.h"
#include "RTC/Transport.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class RtpReceiver
	{
	public:
		class Listener
		{
		public:
			virtual void onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver) = 0;
		};

	public:
		RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Transport* transport, RTC::Transport* rtcpTransport);
		virtual ~RtpReceiver();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);

	public:
		// Passed by argument.
		uint32_t rtpReceiverId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		RTC::Transport* transport = nullptr;
		RTC::Transport* rtcpTransport = nullptr;
	};
}

#endif
