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

		if (this->rtpParameters)
			delete this->rtpParameters;

		// Notify.
		this->notifier->Emit(this->rtpSenderId, "close");

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
			case Channel::Request::MethodId::rtpSender_close:
			{
				uint32_t rtpSenderId = this->rtpSenderId;

				Close();

				MS_DEBUG("RtpSender closed [rtpSenderId:%" PRIu32 "]", rtpSenderId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::rtpSender_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::rtpSender_send:
			{
				// Keep a reference to the previous rtpParameters.
				auto previousRtpParameters = this->rtpParameters;

				try
				{
					this->rtpParameters = new RTC::RtpParameters(request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				// Free the previous rtpParameters.
				delete previousRtpParameters;

				// TODO: may this callback throw if the new parameters are invalid
				// for the Transport(s)?
				// If so, don't dlete previous parameters and keep them.
				this->listener->onRtpSenderParameters(this, this->rtpParameters);

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
