#define MS_CLASS "RTC::RtpCodecParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data, RTC::Scope scope) : scope(scope)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_kind("kind");
		static const Json::StaticString JsonString_name("name");
		static const Json::StaticString JsonString_payloadType("payloadType");
		static const Json::StaticString JsonString_clockRate("clockRate");
		static const Json::StaticString JsonString_maxptime("maxptime");
		static const Json::StaticString JsonString_ptime("ptime");
		static const Json::StaticString JsonString_numChannels("numChannels");
		static const Json::StaticString JsonString_parameters("parameters");
		static const Json::StaticString JsonString_rtcpFeedback("rtcpFeedback");

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecParameters is not an object");

		if (this->scope == RTC::Scope::ROOM_CAPABILITY || this->scope == RTC::Scope::PEER_CAPABILITY)
		{
			// `kind` is mandatory.
			if (!data[JsonString_kind].isString())
				MS_THROW_ERROR("missing RtpCodecParameters.kind");

			std::string kind = data[JsonString_kind].asString();

			// NOTE: This may throw.
			this->kind = RTC::Media::GetKind(kind);
		}

		// `name` is mandatory.
		if (!data[JsonString_name].isString())
			MS_THROW_ERROR("missing RtpCodec.name");

		std::string name = data[JsonString_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		if (data[JsonString_payloadType].isUInt())
		{
			this->payloadType    = (uint8_t)data[JsonString_payloadType].asUInt();
			this->hasPayloadType = true;
		}

		if (this->scope == RTC::Scope::PEER_CAPABILITY)
		{
			if (!this->hasPayloadType)
				MS_THROW_ERROR("missing RtpCodecParameters.payloadType");
		}

		if (this->scope == RTC::Scope::RECEIVE)
		{
			if (!data[JsonString_payloadType].isUInt())
				MS_THROW_ERROR("missing RtpCodecParameters.payloadType");

			this->payloadType    = (uint8_t)data[JsonString_payloadType].asUInt();
			this->hasPayloadType = true;
		}

		// `clockRate` is mandatory.
		if (!data[JsonString_clockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.clockRate");

		this->clockRate = (uint32_t)data[JsonString_clockRate].asUInt();

		// `maxptime` is optional.
		if (data[JsonString_maxptime].isUInt())
			this->maxptime = (uint32_t)data[JsonString_maxptime].asUInt();

		// `ptime` is optional.
		if (data[JsonString_ptime].isUInt())
			this->ptime = (uint32_t)data[JsonString_ptime].asUInt();

		// `numChannels` is optional.
		if (data[JsonString_numChannels].isUInt())
			this->numChannels = (uint32_t)data[JsonString_numChannels].asUInt();

		// `parameters` is optional.
		if (data[JsonString_parameters].isObject())
			this->parameters.Set(data[JsonString_parameters]);

		// `rtcpFeedback` is optional.
		if (data[JsonString_rtcpFeedback].isArray())
		{
			auto& jsonRtcpFeedback = data[JsonString_rtcpFeedback];

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

	Json::Value RtpCodecParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_kind("kind");
		static const Json::StaticString JsonString_name("name");
		static const Json::StaticString JsonString_payloadType("payloadType");
		static const Json::StaticString JsonString_clockRate("clockRate");
		static const Json::StaticString JsonString_maxptime("maxptime");
		static const Json::StaticString JsonString_ptime("ptime");
		static const Json::StaticString JsonString_numChannels("numChannels");
		static const Json::StaticString JsonString_parameters("parameters");
		static const Json::StaticString JsonString_rtcpFeedback("rtcpFeedback");

		Json::Value json(Json::objectValue);

		if (this->scope == RTC::Scope::ROOM_CAPABILITY || this->scope == RTC::Scope::PEER_CAPABILITY)
		{
			// Add `kind`.
			json[JsonString_kind] = RTC::Media::GetJsonString(this->kind);
		}

		// Add `name`.
		json[JsonString_name] = this->mime.GetName();

		if (this->hasPayloadType)
		{
			// Add `payloadType`.
			json[JsonString_payloadType] = (Json::UInt)this->payloadType;
		}

		// Add `clockRate`.
		json[JsonString_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[JsonString_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[JsonString_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels > 1)
			json[JsonString_numChannels] = (Json::UInt)this->numChannels;

		// Add `parameters`.
		json[JsonString_parameters] = this->parameters.toJson();

		// Add `rtcpFeedback`.
		json[JsonString_rtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[JsonString_rtcpFeedback].append(entry.toJson());
		}

		return json;
	}

	bool RtpCodecParameters::Matches(RtpCodecParameters& codec, bool checkPayloadType)
	{
		MS_TRACE();

		static std::string JsonString_packetizationMode = "packetizationMode";

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
				if (this->numChannels != codec.numChannels)
					return false;

				break;
			}

			default:;
		}

		// Per MIME checks.
		switch (this->mime.subtype)
		{
			case RTC::RtpCodecMime::Subtype::H264:
			{
				int32_t packetizationMode      = this->parameters.GetInteger(JsonString_packetizationMode);
				int32_t givenPacketizationMode = codec.parameters.GetInteger(JsonString_packetizationMode);

				if (packetizationMode != givenPacketizationMode)
					return false;

				break;
			}

			default:;
		}

		return true;
	}

	void RtpCodecParameters::ReduceRtcpFeedback(std::vector<RTC::RtcpFeedback>& supportedRtcpFeedback)
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

	inline void RtpCodecParameters::CheckCodec()
	{
		MS_TRACE();

		static std::string JsonString_apt               = "apt";
		static std::string JsonString_packetizationMode = "packetizationMode";

		// Check per MIME parameters and set default values.
		switch (this->mime.subtype)
		{
			case RTC::RtpCodecMime::Subtype::RTX:
			{
				// A RTX codec must have 'apt' parameter.
				if (!this->parameters.HasInteger(JsonString_apt))
					MS_THROW_ERROR("missing apt parameter in RTX RtpCodecParameters");

				break;
			}

			case RTC::RtpCodecMime::Subtype::OPUS:
			{
				// Opus default numChannels is 2.
				if (this->numChannels < 2)
					this->numChannels = 2;

				break;
			}

			case RTC::RtpCodecMime::Subtype::H264:
			{
				// H264 default packetizationMode is 0.
				if (!this->parameters.HasInteger(JsonString_packetizationMode))
					this->parameters.SetInteger(JsonString_packetizationMode, 0);

				break;
			}

			default:;
		}
	}
} // namespace RTC
