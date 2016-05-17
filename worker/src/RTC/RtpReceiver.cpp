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
		static const Json::StaticString k_rtpListenMode("rtpListenMode");

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

		switch (this->rtpListenMode)
		{
			case RtpListenMode::RAW:
				json[k_rtpListenMode] = "raw";
				break;
			case RtpListenMode::OBJECT:
				json[k_rtpListenMode] = "object";
				break;
			default:
				;
		}

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

			case Channel::Request::MethodId::rtpReceiver_rtpListenMode:
			{
				static const Json::StaticString k_mode("mode");

				auto json_mode = request->data[k_mode];
				bool valid = false;

				switch (json_mode.type())
				{
					case Json::stringValue:
					{
						std::string stringValue = json_mode.asString();

						if (stringValue == "raw")
						{
							valid = true;
							this->rtpListenMode = RtpListenMode::RAW;
						}
						else if (stringValue == "object")
						{
							valid = true;
							this->rtpListenMode = RtpListenMode::OBJECT;
						}

						break;
					}

					case Json::booleanValue:
					{
						bool booleanValue = json_mode.asBool();

						if (booleanValue == false)
						{
							valid = true;
							this->rtpListenMode = RtpListenMode::NONE;
						}

						break;
					}

					case Json::nullValue:
					{
						valid = true;
						this->rtpListenMode = RtpListenMode::NONE;

						break;
					}

					default:
						;
				}

				if (!valid)
				{
					MS_ERROR("Request has invalid `data.mode`");

					request->Reject("Request has invalid `data.mode`");
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

	void RtpReceiver::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_object("object");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_marker("marker");
		static const Json::StaticString k_sequenceNumber("sequenceNumber");
		static const Json::StaticString k_timestamp("timestamp");
		static const Json::StaticString k_ssrc("ssrc");

		// TODO: Check if stopped, etc (not yet done)

		// Notify the listener.
		this->listener->onRtpPacket(this, packet);

		// Emit "rtp" event if requeted.
		switch (this->rtpListenMode)
		{
			case RtpListenMode::RAW:
			{
				Json::Value event_data(Json::objectValue);

				event_data[k_class] = "RtpReceiver";

				this->notifier->EmitWithBinary(this->rtpReceiverId, "rtp", event_data, packet->GetRaw(), packet->GetLength());

				break;
			}

			case RtpListenMode::OBJECT:
			{
				Json::Value event_data(Json::objectValue);
				Json::Value json_object(Json::objectValue);

				event_data[k_class] = "RtpReceiver";

				json_object[k_payloadType] = (Json::UInt)packet->GetPayloadType();
				json_object[k_marker] = packet->HasMarker();
				json_object[k_sequenceNumber] = (Json::UInt)packet->GetSequenceNumber();
				json_object[k_timestamp] = (Json::UInt)packet->GetTimestamp();
				json_object[k_ssrc] = (Json::UInt)packet->GetSsrc();

				event_data[k_object] = json_object;

				this->notifier->EmitWithBinary(this->rtpReceiverId, "rtp", event_data, packet->GetPayload(), packet->GetPayloadLength());

				break;
			}

			default:
				;
		}
	}
}
