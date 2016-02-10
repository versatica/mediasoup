#define MS_CLASS "RTC::RtpSender"

#include "RTC/RtpSender.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpSender::RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId) :
		rtpSenderId(rtpSenderId),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();
	}

	RtpSender::~RtpSender()
	{
		MS_TRACE();
	}

	void RtpSender::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		if (this->rtpParameters)
			delete this->rtpParameters;

		// Notify.
		event_data[k_class] = "RtpSender";
		this->notifier->Emit(this->rtpSenderId, "close", event_data);

		// Notify the listener.
		this->listener->onRtpSenderClosed(this);

		delete this;
	}

	Json::Value RtpSender::toJson()
	{
		MS_TRACE();

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_rtpParameters("rtpParameters");

		Json::Value json(Json::objectValue);

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		return json;
	}

	void RtpSender::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::rtpSender_dump:
			{
				Json::Value json = toJson();

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

	void RtpSender::Send(RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		// Free the previous rtpParameters.
		if (this->rtpParameters)
			delete this->rtpParameters;

		this->rtpParameters = rtpParameters;
	}
}
