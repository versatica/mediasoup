#define MS_CLASS "RTC::RtpEncodingParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpEncodingParameters::RtpEncodingParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonString_ssrc("ssrc");
		static const Json::StaticString JsonString_codecPayloadType("codecPayloadType");
		static const Json::StaticString JsonString_fec("fec");
		static const Json::StaticString JsonString_rtx("rtx");
		static const Json::StaticString JsonString_resolutionScale("resolutionScale");
		static const Json::StaticString JsonString_framerateScale("framerateScale");
		static const Json::StaticString JsonString_maxFramerate("maxFramerate");
		static const Json::StaticString JsonString_active("active");
		static const Json::StaticString JsonString_encodingId("encodingId");
		static const Json::StaticString JsonString_dependencyEncodingIds("dependencyEncodingIds");

		if (!data.isObject())
			MS_THROW_ERROR("RtpEncodingParameters is not an object");

		// `codecPayloadType` is optional.
		if (data[JsonString_codecPayloadType].isUInt())
		{
			this->codecPayloadType    = (uint8_t)data[JsonString_codecPayloadType].asUInt();
			this->hasCodecPayloadType = true;
		}

		// `ssrc` is optional.
		if (data[JsonString_ssrc].isUInt())
			this->ssrc = (uint32_t)data[JsonString_ssrc].asUInt();

		// `fec` is optional.
		if (data[JsonString_fec].isObject())
		{
			this->fec    = RtpFecParameters(data[JsonString_fec]);
			this->hasFec = true;
		}

		// `rtx` is optional.
		if (data[JsonString_rtx].isObject())
		{
			this->rtx    = RtpRtxParameters(data[JsonString_rtx]);
			this->hasRtx = true;
		}

		// `resolutionScale` is optional.
		if (data[JsonString_resolutionScale].isDouble())
			this->resolutionScale = data[JsonString_resolutionScale].asDouble();

		// `framerateScale` is optional.
		if (data[JsonString_framerateScale].isDouble())
			this->framerateScale = data[JsonString_framerateScale].asDouble();

		// `maxFramerate` is optional.
		if (data[JsonString_maxFramerate].isUInt())
			this->maxFramerate = (uint32_t)data[JsonString_maxFramerate].asUInt();

		// `active` is optional.
		if (data[JsonString_active].isBool())
			this->active = data[JsonString_active].asBool();

		// `encodingId` is optional.
		if (data[JsonString_encodingId].isString())
			this->encodingId = data[JsonString_encodingId].asString();

		// `dependencyEncodingIds` is optional.
		if (data[JsonString_dependencyEncodingIds].isArray())
		{
			auto& jsonArray = data[JsonString_dependencyEncodingIds];

			for (auto& entry : jsonArray)
			{
				// Append to the dependencyEncodingIds vector.
				if (entry.isString())
					this->dependencyEncodingIds.push_back(entry.asString());
			}
		}
	}

	Json::Value RtpEncodingParameters::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_ssrc("ssrc");
		static const Json::StaticString JsonString_codecPayloadType("codecPayloadType");
		static const Json::StaticString JsonString_fec("fec");
		static const Json::StaticString JsonString_rtx("rtx");
		static const Json::StaticString JsonString_resolutionScale("resolutionScale");
		static const Json::StaticString JsonString_framerateScale("framerateScale");
		static const Json::StaticString JsonString_maxFramerate("maxFramerate");
		static const Json::StaticString JsonString_active("active");
		static const Json::StaticString JsonString_encodingId("encodingId");
		static const Json::StaticString JsonString_dependencyEncodingIds("dependencyEncodingIds");

		Json::Value json(Json::objectValue);

		// Add `codecPayloadType`.
		if (this->hasCodecPayloadType)
			json[JsonString_codecPayloadType] = (Json::UInt)this->codecPayloadType;

		// Add `ssrc`.
		if (this->ssrc)
			json[JsonString_ssrc] = (Json::UInt)this->ssrc;

		// Add `fec`
		if (this->hasFec)
			json[JsonString_fec] = this->fec.toJson();

		// Add `rtx`
		if (this->hasRtx)
			json[JsonString_rtx] = this->rtx.toJson();

		// Add `resolutionScale` (if different than the default value).
		if (this->resolutionScale != 1.0)
			json[JsonString_resolutionScale] = this->resolutionScale;

		// Add `framerateScale` (if different than the default value).
		if (this->framerateScale != 1.0)
			json[JsonString_framerateScale] = this->framerateScale;

		// Add `maxFramerate`.
		if (this->maxFramerate)
			json[JsonString_maxFramerate] = (Json::UInt)this->maxFramerate;

		// Add `active`.
		json[JsonString_active] = this->active;

		// Add `encodingId`.
		if (!this->encodingId.empty())
			json[JsonString_encodingId] = this->encodingId;

		// Add `dependencyEncodingIds` (if any).
		if (!this->dependencyEncodingIds.empty())
		{
			json[JsonString_dependencyEncodingIds] = Json::arrayValue;

			for (auto& entry : this->dependencyEncodingIds)
			{
				json[JsonString_dependencyEncodingIds].append(entry);
			}
		}

		return json;
	}
} // namespace RTC
