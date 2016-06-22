#define MS_CLASS "RTC::RtpSender"

#include "RTC/RtpSender.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpSender::RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind) :
		rtpSenderId(rtpSenderId),
		kind(kind),
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

		if (this->rtpParameters)
			delete this->rtpParameters;

		Json::Value event_data(Json::objectValue);

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
		static const Json::StaticString k_rtpSenderId("rtpSenderId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_hasTransport("hasTransport");
		static const Json::StaticString k_available("available");

		Json::Value json(Json::objectValue);

		json[k_rtpSenderId] = (Json::UInt)this->rtpSenderId;

		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		json[k_hasTransport] = this->transport ? true : false;

		json[k_available] = this->available;

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

	void RtpSender::SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities)
	{
		MS_TRACE();

		this->peerCapabilities = peerCapabilities;
	}

	void RtpSender::Send(RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_available("available");

		auto previousRtpParameters = this->rtpParameters;

		// Free the previous rtpParameters.
		if (previousRtpParameters)
			delete this->rtpParameters;

		// Clone given RTP parameters so we manage our own sender parameters.
		this->rtpParameters = new RTC::RtpParameters(rtpParameters);

		// TODO: Must check new parameters and:
		// - remove unsuported capabilities,
		// - set this->available if at least there is a capable encoding.
		// For now make it easy:
		this->available = true;

		// Emit "parameterschange" if those are new parameters.
		if (previousRtpParameters)
		{
			Json::Value event_data(Json::objectValue);

			event_data[k_class] = "RtpSender";
			event_data[k_rtpParameters] = this->rtpParameters->toJson();
			event_data[k_available] = this->available;

			this->notifier->Emit(this->rtpSenderId, "parameterschange", event_data);
		}
	}
}
