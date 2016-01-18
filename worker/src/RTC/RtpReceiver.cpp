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

		// Notify the listener and also the rtpListener and rtcpListener.
		this->listener->onRtpReceiverClosed(this);
		if (this->rtpListener)
			this->rtpListener->onRtpReceiverClosed(this);
		if (this->rtcpListener)
			this->rtcpListener->onRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::toJson()
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

				request->Accept();

				// Notify the rtpListener and rtcpListener.
				if (this->rtpListener)
					this->rtpListener->onRtpListenerParameters(this, this->rtpParameters);
				if (this->rtcpListener)
					this->rtcpListener->onRtpListenerParameters(this, this->rtpParameters);

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
