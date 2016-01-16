#define MS_CLASS "RTC::RtpReceiver"

#include "RTC/RtpReceiver.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpReceiver::RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId) :
		rtpReceiverId(rtpReceiverId),
		listener(listener),
		notifier(notifier)
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

		if (this->rtpParameters)
			delete this->rtpParameters;

		// Notify.
		this->notifier->Emit(this->rtpReceiverId, "close");

		// Notify the listener.
		this->listener->onRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_rtpParameters("rtpParameters");

		Json::Value data(Json::objectValue);

		// TODO

		if (this->rtpParameters)
			data[k_rtpParameters] = this->rtpParameters->toJson();

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
				// If rtpParameteres was already set, delete it.
				if (this->rtpParameters)
				{
					delete this->rtpParameters;
					this->rtpParameters = nullptr;
				}

				try
				{
					this->rtpParameters = new RTC::RtpParameters(request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				request->Accept();

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
