#define MS_CLASS "RTC::RtpReceiver"

#include "RTC/RtpReceiver.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpReceiver::RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, std::string& kind) :
		rtpReceiverId(rtpReceiverId),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();

		if (kind == "audio")
			this->kind = RTC::RtpKind::AUDIO;
		else if (kind == "video")
			this->kind = RTC::RtpKind::VIDEO;
		else
			MS_THROW_ERROR("unknown `kind`");
	}

	RtpReceiver::~RtpReceiver()
	{
		MS_TRACE();
	}

	void RtpReceiver::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		if (this->rtpParameters)
			delete this->rtpParameters;

		// Notify.
		event_data[k_class] = "RtpReceiver";
		this->notifier->Emit(this->rtpReceiverId, "close", event_data);

		// Notify the listener.
		this->listener->onRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::toJson()
	{
		MS_TRACE();

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_rtpReceiverId("rtpReceiverId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString v_audio("audio");
		static const Json::StaticString v_video("video");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_hasTransport("hasTransport");

		Json::Value json(Json::objectValue);

		json[k_rtpReceiverId] = (Json::UInt)this->rtpReceiverId;

		switch (this->kind)
		{
			case RTC::RtpKind::AUDIO:
				json[k_kind] = v_audio;
				break;
			case RTC::RtpKind::VIDEO:
				json[k_kind] = v_video;
				break;
		}

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		json[k_hasTransport] = this->transport ? true : false;

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
					this->rtpParameters = RTC::RtpParameters::Factory(this->kind, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());

					return;
				}

				// NOTE: this may throw. If so keep the current parameters.
				try
				{
					this->listener->onRtpReceiverParameters(this, this->rtpParameters);
				}
				catch (const MediaSoupError &error)
				{
					this->rtpParameters = previousRtpParameters;
					request->Reject(error.what());

					return;
				}

				// Free the previous rtpParameters.
				delete previousRtpParameters;

				Json::Value data = this->rtpParameters->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_listenForRtp:
			{
				static const Json::StaticString k_enabled("enabled");

				auto json_enabled = request->data[k_enabled];

				if (!json_enabled.isBool())
				{
					MS_ERROR("Request has not boolean `data.enabled`");

					request->Reject("Request has not boolean `data.enabled`");
					return;
				}

				this->listenForRtp = json_enabled.asBool();

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

	void RtpReceiver::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// TODO: Check if stopped, etc (not yet done)

		// Notify the listener.
		this->listener->onRtpPacket(this, packet);

		if (this->listenForRtp)
		{
			// Send a JSON event followed by binary data (the RTP packet).

			static const Json::StaticString k_class("class");

			Json::Value event_data(Json::objectValue);

			// Notify binary event.
			event_data[k_class] = "RtpReceiver";

			this->notifier->EmitWithBinary(this->rtpReceiverId, "rtp", event_data, packet->GetRaw(), packet->GetLength());
		}
	}
}
