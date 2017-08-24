#define MS_CLASS "RTC::RtpCodecCapability"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpCodecCapability::RtpCodecCapability(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringMimeType{ "mimeType" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringClockRate{ "clockRate" };
		static const Json::StaticString JsonStringMaxptime{ "maxptime" };
		static const Json::StaticString JsonStringPtime{ "ptime" };
		static const Json::StaticString JsonStringNumChannels{ "channels" };
		static const Json::StaticString JsonStringParameters{ "parameters" };
		static const Json::StaticString JsonStringRtcpFeedback{ "rtcpFeedback" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecCapability is not an object");

		// `kind` is mandatory.
		if (!data[JsonStringKind].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.kind");

		std::string kind = data[JsonStringKind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// `mimeType` is mandatory.
		if (!data[JsonStringMimeType].isString())
			MS_THROW_ERROR("missing RtpCodec.mimeType");

		std::string mimeType = data[JsonStringMimeType].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetMimeType(mimeType);

		if (!data[JsonStringPayloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecCapability.payloadType");

		this->payloadType = static_cast<uint8_t>(data[JsonStringPayloadType].asUInt());

		// `clockRate` is mandatory.
		if (!data[JsonStringClockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodecCapability.clockRate");

		this->clockRate = uint32_t{ data[JsonStringClockRate].asUInt() };

		// `maxptime` is optional.
		if (data[JsonStringMaxptime].isUInt())
			this->maxptime = uint32_t{ data[JsonStringMaxptime].asUInt() };

		// `ptime` is optional.
		if (data[JsonStringPtime].isUInt())
			this->ptime = uint32_t{ data[JsonStringPtime].asUInt() };

		// `channels` is optional.
		if (data[JsonStringNumChannels].isUInt())
			this->channels = uint32_t{ data[JsonStringNumChannels].asUInt() };

		// `parameters` is optional.
		if (data[JsonStringParameters].isObject())
			this->parameters.Set(data[JsonStringParameters]);

		// `rtcpFeedback` is optional.
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

	Json::Value RtpCodecCapability::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringKind{ "kind" };
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

		// Add `kind`.
		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		// Add `mimeType`.
		json[JsonStringMimeType] = this->mime.ToString();

		// Add `name`.
		json[JsonStringName] = this->mime.GetName();

		// Add `payloadType`.
		json[JsonStringPayloadType] = Json::UInt{ this->payloadType };

		// Add `clockRate`.
		json[JsonStringClockRate] = Json::UInt{ this->clockRate };

		// Add `maxptime`.
		if (this->maxptime != 0u)
			json[JsonStringMaxptime] = Json::UInt{ this->maxptime };

		// Add `ptime`.
		if (this->ptime != 0u)
			json[JsonStringPtime] = Json::UInt{ this->ptime };

		// Add `channels`.
		if (this->channels > 1)
			json[JsonStringNumChannels] = Json::UInt{ this->channels };

		// Add `parameters`.
		json[JsonStringParameters] = this->parameters.ToJson();

		// Add `rtcpFeedback`.
		json[JsonStringRtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[JsonStringRtcpFeedback].append(entry.ToJson());
		}

		return json;
	}

	bool RtpCodecCapability::Matches(RtpCodecCapability& codec, bool checkPayloadType)
	{
		MS_TRACE();

		static std::string jsonStringPacketizationMode{ "packetizationMode" };

		// MIME must match.
		if (this->mime != codec.mime)
			return false;

		if (checkPayloadType)
		{
			if (this->payloadType != codec.payloadType)
				return false;
		}

		// Clock rate must match.
		if (this->clockRate != codec.clockRate)
			return false;

		// Per kind checks.
		switch (this->kind)
		{
			case RTC::Media::Kind::AUDIO:
			{
				// Num channels must match.
				if (this->channels != codec.channels)
					return false;

				break;
			}

			default:;
		}

		// Per MIME checks.
		switch (this->mime.subtype)
		{
			case RTC::RtpCodecMimeType::Subtype::H264:
			{
				int32_t packetizationMode      = this->parameters.GetInteger(jsonStringPacketizationMode);
				int32_t givenPacketizationMode = codec.parameters.GetInteger(jsonStringPacketizationMode);

				if (packetizationMode != givenPacketizationMode)
					return false;

				break;
			}

			default:;
		}

		return true;
	}

	void RtpCodecCapability::ReduceRtcpFeedback(std::vector<RTC::RtcpFeedback>& supportedRtcpFeedback)
	{
		MS_TRACE();

		std::vector<RTC::RtcpFeedback> updatedRtcpFeedback;

		for (auto& rtcpFeedbackItem : this->rtcpFeedback)
		{
			for (auto& supportedRtcpFeedbackItem : supportedRtcpFeedback)
			{
				if (rtcpFeedbackItem.type == supportedRtcpFeedbackItem.type &&
				    rtcpFeedbackItem.parameter == supportedRtcpFeedbackItem.parameter)
				{
					updatedRtcpFeedback.push_back(supportedRtcpFeedbackItem);

					break;
				}
			}
		}

		this->rtcpFeedback = updatedRtcpFeedback;
	}

	inline void RtpCodecCapability::CheckCodec()
	{
		MS_TRACE();

		static std::string jsonStringApt{ "apt" };
		static std::string jsonStringPacketizationMode{ "packetizationMode" };

		// Check per MIME parameters and set default values.
		switch (this->mime.subtype)
		{
			case RTC::RtpCodecMimeType::Subtype::RTX:
			{
				// A RTX codec must have 'apt' parameter.
				if (!this->parameters.HasInteger(jsonStringApt))
					MS_THROW_ERROR("missing apt parameter in RTX RtpCodecCapability");

				break;
			}

			case RTC::RtpCodecMimeType::Subtype::OPUS:
			{
				// Opus default channels is 2.
				if (this->channels < 2)
					this->channels = 2;

				break;
			}

			case RTC::RtpCodecMimeType::Subtype::H264:
			{
				// H264 default packetizationMode is 0.
				if (!this->parameters.HasInteger(jsonStringPacketizationMode))
					this->parameters.SetInteger(jsonStringPacketizationMode, 0);

				break;
			}

			default:;
		}
	}
} // namespace RTC
