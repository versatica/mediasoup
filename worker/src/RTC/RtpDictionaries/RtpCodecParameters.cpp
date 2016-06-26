#define MS_CLASS "RTC::RtpCodecParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(Json::Value& data, RTC::Scope scope) :
		scope(scope)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_parameters("parameters");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");

		if (!data.isObject())
			MS_THROW_ERROR("RtpCodecParameters is not an object");

		if (this->scope == RTC::Scope::ROOM_CAPABILITY ||
			this->scope == RTC::Scope::PEER_CAPABILITY)
		{
			// `kind` is mandatory.
			if (!data[k_kind].isString())
				MS_THROW_ERROR("missing RtpCodecParameters.kind");

			std::string kind = data[k_kind].asString();

			// NOTE: This may throw.
			this->kind = RTC::Media::GetKind(kind);
		}

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing RtpCodec.name");

		std::string name = data[k_name].asString();

		// Set MIME field.
		// NOTE: This may throw.
		this->mime.SetName(name);

		if (data[k_payloadType].isUInt())
		{
			this->payloadType = (uint8_t)data[k_payloadType].asUInt();
			this->hasPayloadType = true;
		}

		if (this->scope == RTC::Scope::PEER_CAPABILITY ||
			this->scope == RTC::Scope::RECEIVE)
		{
			if (!this->hasPayloadType)
				MS_THROW_ERROR("missing RtpCodecParameters.payloadType");
		}

		// `clockRate` is mandatory.
		if (!data[k_clockRate].isUInt())
			MS_THROW_ERROR("missing RtpCodecParameters.clockRate");

		this->clockRate = (uint32_t)data[k_clockRate].asUInt();

		// `maxptime` is optional.
		if (data[k_maxptime].isUInt())
			this->maxptime = (uint32_t)data[k_maxptime].asUInt();

		// `ptime` is optional.
		if (data[k_ptime].isUInt())
			this->ptime = (uint32_t)data[k_ptime].asUInt();

		// `numChannels` is optional.
		if (data[k_numChannels].isUInt())
			this->numChannels = (uint32_t)data[k_numChannels].asUInt();

		// `parameters` is optional.
		if (data[k_parameters].isObject())
			this->parameters.Set(data[k_parameters]);

		if (this->scope == RTC::Scope::RECEIVE)
		{
			// `rtx` is optional.
			if (data[k_rtx].isObject())
			{
				this->rtx = RTC::RTCRtpCodecRtxParameters(data[k_rtx]);
				this->hasRtx = true;
			}
		}

		if (this->scope == RTC::Scope::PEER_CAPABILITY ||
			this->scope == RTC::Scope::RECEIVE)
		{
			// `rtcpFeedback` is optional.
			if (data[k_rtcpFeedback].isArray())
			{
				auto& json_rtcpFeedback = data[k_rtcpFeedback];

				for (Json::UInt i = 0; i < json_rtcpFeedback.size(); i++)
				{
					RTC::RtcpFeedback rtcpFeedback(json_rtcpFeedback[i]);

					// Append to the rtcpFeedback vector.
					this->rtcpFeedback.push_back(rtcpFeedback);
				}
			}
		}

		// Check codec.
		CheckCodec();
	}

	Json::Value RtpCodecParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_maxptime("maxptime");
		static const Json::StaticString k_ptime("ptime");
		static const Json::StaticString k_numChannels("numChannels");
		static const Json::StaticString k_parameters("parameters");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");

		Json::Value json(Json::objectValue);

		if (this->scope == RTC::Scope::ROOM_CAPABILITY ||
			this->scope == RTC::Scope::PEER_CAPABILITY)
		{
			// Add `kind`.
			json[k_kind] = RTC::Media::GetJsonString(this->kind);
		}

		// Add `name`.
		json[k_name] = this->mime.GetName();

		if (this->hasPayloadType)
		{
			// Add `payloadType`.
			json[k_payloadType] = (Json::UInt)this->payloadType;
		}

		// Add `clockRate`.
		json[k_clockRate] = (Json::UInt)this->clockRate;

		// Add `maxptime`.
		if (this->maxptime)
			json[k_maxptime] = (Json::UInt)this->maxptime;

		// Add `ptime`.
		if (this->ptime)
			json[k_ptime] = (Json::UInt)this->ptime;

		// Add `numChannels`.
		if (this->numChannels > 1)
			json[k_numChannels] = (Json::UInt)this->numChannels;

		// Add `parameters`.
		json[k_parameters] = this->parameters.toJson();

		if (this->scope == RTC::Scope::RECEIVE)
		{
			// Add `rtx`
			if (this->hasRtx)
				json[k_rtx] = this->rtx.toJson();
		}

		if (this->scope == RTC::Scope::PEER_CAPABILITY ||
			this->scope == RTC::Scope::RECEIVE)
		{
			// Add `rtcpFeedback`.
			json[k_rtcpFeedback] = Json::arrayValue;

			for (auto& entry : this->rtcpFeedback)
			{
				json[k_rtcpFeedback].append(entry.toJson());
			}
		}

		return json;
	}

	bool RtpCodecParameters::Matches(RtpCodecParameters& codec, bool checkPayloadType)
	{
		MS_TRACE();

		static std::string k_packetizationMode = "packetizationMode";

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
				// Num channels must match
				if (this->numChannels != codec.numChannels)
					return false;

				break;
			}

			default:
				;
		}

		// Per MIME checks.
		switch (this->mime.subtype)
		{
			case RTC::RtpCodecMime::Subtype::H264:
			{
				if (this->parameters.GetInteger(k_packetizationMode) !=
					codec.parameters.GetInteger(k_packetizationMode))
				{
					return false;
				}

				break;
			}

			default:
				;
		}

		return true;
	}

	inline
	void RtpCodecParameters::CheckCodec()
	{
		MS_TRACE();

		static std::string k_packetizationMode = "packetizationMode";

		// Check per MIME parameters and set default values.

		switch (this->mime.subtype)
		{
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

				if (!this->parameters.HasInteger(k_packetizationMode))
					this->parameters.SetInteger(k_packetizationMode, 0);

				break;
			}

			default:
				;
		}
	}
}
