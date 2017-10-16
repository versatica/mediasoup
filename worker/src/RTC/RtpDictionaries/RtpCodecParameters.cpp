#define MS_CLASS "RTC::RtpCodecParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringMimeType{ "mimeType" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringClockRate{ "clockRate" };
		static const Json::StaticString JsonStringMaxptime{ "maxptime" };
		static const Json::StaticString JsonStringPtime{ "ptime" };
		static const Json::StaticString JsonStringNumChannels{ "channels" };
		static const Json::StaticString JsonStringParameters{ "parameters" };
		static const Json::StaticString JsonStringRtcpFeedback{ "rtcpFeedback" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecParameters is not an object");

		// mimeType is mandatory.
		if (!data[JsonStringMimeType].isString())
			MS_THROW_ERROR("missing RtpCodec.mimeType");

		std::string mimeType = data[JsonStringMimeType].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mimeType.SetMimeType(mimeType);

		if (!data[JsonStringPayloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.payloadType");

		this->payloadType = static_cast<uint8_t>(data[JsonStringPayloadType].asUInt());

		// clockRate is mandatory.
		if (!data[JsonStringClockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.clockRate");

		this->clockRate = uint32_t{ data[JsonStringClockRate].asUInt() };

		// maxptime is optional.
		if (data[JsonStringMaxptime].isUInt())
			this->maxptime = uint32_t{ data[JsonStringMaxptime].asUInt() };

		// ptime is optional.
		if (data[JsonStringPtime].isUInt())
			this->ptime = uint32_t{ data[JsonStringPtime].asUInt() };

		// channels is optional.
		if (data[JsonStringNumChannels].isUInt())
			this->channels = uint32_t{ data[JsonStringNumChannels].asUInt() };

		// parameters is optional.
		if (data[JsonStringParameters].isObject())
			this->parameters.Set(data[JsonStringParameters]);

		// rtcpFeedback is optional.
		if (data[JsonStringRtcpFeedback].isArray())
		{
			auto& jsonRtcpFeedback = data[JsonStringRtcpFeedback];

			for (auto& i : jsonRtcpFeedback)
			{
				RTC::RtcpFeedback rtcpFeedback(i);

				// Append to the rtcpFeedback vector.
				this->rtcpFeedback.push_back(rtcpFeedback);
			}
		}

		// Check codec.
		CheckCodec();
	}

	Json::Value RtpCodecParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringMimeType{ "mimeType" };
		static const Json::StaticString JsonStringName{ "name" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringClockRate{ "clockRate" };
		static const Json::StaticString JsonStringMaxptime{ "maxptime" };
		static const Json::StaticString JsonStringPtime{ "ptime" };
		static const Json::StaticString JsonStringNumChannels{ "channels" };
		static const Json::StaticString JsonStringParameters{ "parameters" };
		static const Json::StaticString JsonStringRtcpFeedback{ "rtcpFeedback" };

		Json::Value json(Json::objectValue);

		// Add mimeType.
		json[JsonStringMimeType] = this->mimeType.ToString();

		// Add name.
		json[JsonStringName] = this->mimeType.GetName();

		// Add payloadType.
		json[JsonStringPayloadType] = Json::UInt{ this->payloadType };

		// Add clockRate.
		json[JsonStringClockRate] = Json::UInt{ this->clockRate };

		// Add maxptime.
		if (this->maxptime != 0u)
			json[JsonStringMaxptime] = Json::UInt{ this->maxptime };

		// Add ptime.
		if (this->ptime != 0u)
			json[JsonStringPtime] = Json::UInt{ this->ptime };

		// Add channels.
		if (this->channels > 1)
			json[JsonStringNumChannels] = Json::UInt{ this->channels };

		// Add parameters.
		json[JsonStringParameters] = this->parameters.ToJson();

		// Add rtcpFeedback.
		json[JsonStringRtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[JsonStringRtcpFeedback].append(entry.ToJson());
		}

		return json;
	}

	inline void RtpCodecParameters::CheckCodec()
	{
		MS_TRACE();

		static std::string jsonStringApt{ "apt" };

		// Check per MIME parameters and set default values.
		switch (this->mimeType.subtype)
		{
			case RTC::RtpCodecMimeType::Subtype::RTX:
			{
				// A RTX codec must have 'apt' parameter.
				if (!this->parameters.HasInteger(jsonStringApt))
					MS_THROW_ERROR("missing apt parameter in RTX RtpCodecParameters");

				break;
			}

			default:;
		}
	}
} // namespace RTC
