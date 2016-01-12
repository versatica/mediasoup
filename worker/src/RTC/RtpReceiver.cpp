#define MS_CLASS "RTC::RtpReceiver"

#include "RTC/RtpReceiver.h"
// #include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpReceiver::RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Transport* transport, RTC::Transport* rtcpTransport) :
		rtpReceiverId(rtpReceiverId),
		listener(listener),
		notifier(notifier),
		transport(transport),
		rtcpTransport(rtcpTransport)
	{
		MS_TRACE();
	}

	RtpReceiver::~RtpReceiver()
	{
		MS_TRACE();
	}

	void RtpReceiver::Close()
	{
		MS_TRACE();

		// Notify.
		this->notifier->Emit(this->rtpReceiverId, "close");

		// Notify the listener.
		this->listener->onRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::toJson()
	{
		MS_TRACE();

		Json::Value data;

		data = Json::nullValue;

		return data;
	}

	void RtpReceiver::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::rtpReceiver_close:
			{
				uint32_t rtpReceiverId = this->rtpReceiverId;

				Close();

				MS_DEBUG("RtpReceiver closed [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_receive:
			{
				// TODO
				Json::Value json = 1234;

				request->Accept(json);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}
}
