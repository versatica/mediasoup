#define MS_CLASS "RTC::RtpListener"

#include "RTC/RtpListener.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpListener::RtpListener()
	{
		MS_TRACE();
	}

	RtpListener::~RtpListener()
	{
		MS_TRACE();
	}

	void RtpListener::Close()
	{
		MS_TRACE();

		delete this;
	}

	void RtpListener::AddRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		auto it = this->rtpReceivers.find(rtpReceiver->rtpReceiverId);
		MS_ASSERT(it == this->rtpReceivers.end(), "given RtpReceiver already exists");

		this->rtpReceivers[rtpReceiver->rtpReceiverId] = rtpReceiver;
	}

	RTC::RtpReceiver* RtpListener::GetRtpReceiver(RTC::RTPPacket* rtpPacket)
	{
		MS_TRACE();

		// TODO: yeah

		return nullptr;
	}
}
