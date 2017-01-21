#define MS_CLASS "RTC::RtpEncodingParameters"
// #define MS_LOG_DEV

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpEncodingParameters::RtpEncodingParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_codecPayloadType("codecPayloadType");
		static const Json::StaticString k_fec("fec");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_resolutionScale("resolutionScale");
		static const Json::StaticString k_framerateScale("framerateScale");
		static const Json::StaticString k_maxFramerate("maxFramerate");
		static const Json::StaticString k_active("active");
		static const Json::StaticString k_encodingId("encodingId");
		static const Json::StaticString k_dependencyEncodingIds("dependencyEncodingIds");

		if (!data.isObject())
			MS_THROW_ERROR("RtpEncodingParameters is not an object");

		// `codecPayloadType` is optional.
		if (data[k_codecPayloadType].isUInt())
		{
			this->codecPayloadType = (uint8_t)data[k_codecPayloadType].asUInt();
			this->hasCodecPayloadType = true;
		}

		// `ssrc` is optional.
		if (data[k_ssrc].isUInt())
			this->ssrc = (uint32_t)data[k_ssrc].asUInt();

		// `fec` is optional.
		if (data[k_fec].isObject())
		{
			this->fec = RtpFecParameters(data[k_fec]);
			this->hasFec = true;
		}

		// `rtx` is optional.
		if (data[k_rtx].isObject())
		{
			this->rtx = RtpRtxParameters(data[k_rtx]);
			this->hasRtx = true;
		}

		// `resolutionScale` is optional.
		if (data[k_resolutionScale].isDouble())
			this->resolutionScale = data[k_resolutionScale].asDouble();

		// `framerateScale` is optional.
		if (data[k_framerateScale].isDouble())
			this->framerateScale = data[k_framerateScale].asDouble();

		// `maxFramerate` is optional.
		if (data[k_maxFramerate].isUInt())
			this->maxFramerate = (uint32_t)data[k_maxFramerate].asUInt();

		// `active` is optional.
		if (data[k_active].isBool())
			this->active = data[k_active].asBool();

		// `encodingId` is optional.
		if (data[k_encodingId].isString())
			this->encodingId = data[k_encodingId].asString();

		// `dependencyEncodingIds` is optional.
		if (data[k_dependencyEncodingIds].isArray())
		{
			auto& json_array = data[k_dependencyEncodingIds];

			for (Json::UInt i = 0; i < json_array.size(); ++i)
			{
				auto& entry = json_array[i];

				// Append to the dependencyEncodingIds vector.
				if (entry.isString())
					this->dependencyEncodingIds.push_back(entry.asString());
			}
		}
	}

	Json::Value RtpEncodingParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_codecPayloadType("codecPayloadType");
		static const Json::StaticString k_fec("fec");
		static const Json::StaticString k_rtx("rtx");
		static const Json::StaticString k_resolutionScale("resolutionScale");
		static const Json::StaticString k_framerateScale("framerateScale");
		static const Json::StaticString k_maxFramerate("maxFramerate");
		static const Json::StaticString k_active("active");
		static const Json::StaticString k_encodingId("encodingId");
		static const Json::StaticString k_dependencyEncodingIds("dependencyEncodingIds");

		Json::Value json(Json::objectValue);

		// Add `codecPayloadType`.
		if (this->hasCodecPayloadType)
			json[k_codecPayloadType] = (Json::UInt)this->codecPayloadType;

		// Add `ssrc`.
		if (this->ssrc)
			json[k_ssrc] = (Json::UInt)this->ssrc;

		// Add `fec`
		if (this->hasFec)
			json[k_fec] = this->fec.toJson();

		// Add `rtx`
		if (this->hasRtx)
			json[k_rtx] = this->rtx.toJson();

		// Add `resolutionScale` (if different than the default value).
		if (this->resolutionScale != 1.0)
			json[k_resolutionScale] = this->resolutionScale;

		// Add `framerateScale` (if different than the default value).
		if (this->framerateScale != 1.0)
			json[k_framerateScale] = this->framerateScale;

		// Add `maxFramerate`.
		if (this->maxFramerate)
			json[k_maxFramerate] = (Json::UInt)this->maxFramerate;

		// Add `active`.
		json[k_active] = this->active;

		// Add `encodingId`.
		if (!this->encodingId.empty())
			json[k_encodingId] = this->encodingId;

		// Add `dependencyEncodingIds` (if any).
		if (!this->dependencyEncodingIds.empty())
		{
			json[k_dependencyEncodingIds] = Json::arrayValue;

			for (auto& entry : this->dependencyEncodingIds)
			{
				json[k_dependencyEncodingIds].append(entry);
			}
		}

		return json;
	}
}
