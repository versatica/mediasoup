#define MS_CLASS "RTC::RtpCodecParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data, bool isRoomCodec) :
		RtpCodec(data),
		isRoomCodec(isRoomCodec)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");

		// Not a room codec.
		if (!this->isRoomCodec)
		{
			// `payloadType` is mandatory.
			if (!data[k_payloadType].isUInt())
				MS_THROW_ERROR("missing RtpCodecParameters.payloadType");

			this->payloadType = (uint8_t)data[k_payloadType].asUInt();

			// `rtx` is optional.
			if (data[k_rtx].isObject())
			{
				this->rtx = RTCRtpCodecRtxParameters(data[k_rtx]);
				this->hasRtx = true;
			}

			// `rtcpFeedback` is optional.
			if (data[k_rtcpFeedback].isArray())
			{
				auto& json_rtcpFeedback = data[k_rtcpFeedback];

				for (Json::UInt i = 0; i < json_rtcpFeedback.size(); i++)
				{
					RtcpFeedback rtcpFeedback(json_rtcpFeedback[i]);

					// Append to the rtcpFeedback vector.
					this->rtcpFeedback.push_back(rtcpFeedback);
				}
			}
		}
		// Room codec.
		else
		{
			// `kind` is mandatory.
			if (!data[k_kind].isString())
				MS_THROW_ERROR("missing RtpCodecParameters.kind");

			std::string kind = data[k_kind].asString();

			// NOTE: This may throw.
			this->kind = RTC::Media::GetKind(kind);

			// It must not be a feature codec.
			if (this->mime.IsFeatureCodec())
				MS_THROW_ERROR("can not be a feature codec");
		}

		// TODO: Check per MIME parameters and set default values.
		// For example, H264 default packetizationMode is 0.
	}

	Json::Value RtpCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");

		Json::Value json(Json::objectValue);

		// Call the parent method.
		RtpCodec::toJson(json);

		// Not a room codec.
		if (!this->isRoomCodec)
		{
			// Add `payloadType`.
			json[k_payloadType] = (Json::UInt)this->payloadType;

			// Add `rtx`
			if (this->hasRtx)
				json[k_rtx] = this->rtx.toJson();

			// Add `rtcpFeedback`.
			json[k_rtcpFeedback] = Json::arrayValue;

			for (auto& entry : this->rtcpFeedback)
			{
				json[k_rtcpFeedback].append(entry.toJson());
			}
		}
		// Room codec.
		else
		{
			// Add `kind`.
			json[k_kind] = RTC::Media::GetJsonString(this->kind);
		}

		return json;
	}
}
