#define MS_CLASS "RTC::RtpCodecCapability"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpCodecCapability::RtpCodecCapability(Json::Value& data) :
		RtpCodec(data)
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_preferredPayloadType("preferredPayloadType");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_maxTemporalLayers("maxTemporalLayers");
		static const Json::StaticString k_maxSpatialLayers("maxSpatialLayers");
		static const Json::StaticString k_svcMultiStreamSupport("svcMultiStreamSupport");

		// `kind` is mandatory.
		if (!data[k_kind].isString())
			MS_THROW_ERROR("missing RtpCodecCapability.kind");

		std::string kind = data[k_kind].asString();

		// NOTE: This may throw.
		this->kind = RTC::Media::GetKind(kind);

		// It can not be empty kind (all).
		if (this->kind == RTC::Media::Kind::ALL)
			MS_THROW_ERROR("invalid empty RtpCodecCapability.kind");

		// `preferredPayloadType` is mandatory.
		if (!data[k_preferredPayloadType].isUInt())
			MS_THROW_ERROR("missing RtpCodecCapability.preferredPayloadType");

		this->preferredPayloadType = (uint8_t)data[k_preferredPayloadType].asUInt();

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

		// `maxTemporalLayers` is optional.
		if (data[k_maxTemporalLayers].isUInt())
			this->maxTemporalLayers = (uint16_t)data[k_maxTemporalLayers].asUInt();

		// `maxSpatialLayers` is optional.
		if (data[k_maxSpatialLayers].isUInt())
			this->maxSpatialLayers = (uint16_t)data[k_maxSpatialLayers].asUInt();

		// `svcMultiStreamSupport` is optional.
		if (data[k_svcMultiStreamSupport].isBool())
			this->svcMultiStreamSupport = data[k_svcMultiStreamSupport].asBool();

		// Check codec capability.
		CheckCodecCapability();
	}

	Json::Value RtpCodecCapability::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_preferredPayloadType("preferredPayloadType");
		static const Json::StaticString k_rtcpFeedback("rtcpFeedback");
		static const Json::StaticString k_maxTemporalLayers("maxTemporalLayers");
		static const Json::StaticString k_maxSpatialLayers("maxSpatialLayers");
		static const Json::StaticString k_svcMultiStreamSupport("svcMultiStreamSupport");

		Json::Value json(Json::objectValue);

		// Call the parent method.
		RtpCodec::toJson(json);

		// Add `kind`.
		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		// Add `preferredPayloadType`.
		json[k_preferredPayloadType] = (Json::UInt)this->preferredPayloadType;

		// Add `rtcpFeedback`.
		json[k_rtcpFeedback] = Json::arrayValue;

		for (auto& entry : this->rtcpFeedback)
		{
			json[k_rtcpFeedback].append(entry.toJson());
		}

		// Add `maxTemporalLayers` (if set).
		if (this->maxTemporalLayers)
			json[k_maxTemporalLayers] = (Json::UInt)this->maxTemporalLayers;

		// Add `maxSpatialLayers` (if set).
		if (this->maxSpatialLayers)
			json[k_maxSpatialLayers] = (Json::UInt)this->maxSpatialLayers;

		// Add `svcMultiStreamSupport` (if set).
		if (this->svcMultiStreamSupport)
			json[k_svcMultiStreamSupport] = true;

		return json;
	}

	bool RtpCodecCapability::MatchesCodec(RtpCodecParameters& codec)
	{
		MS_TRACE();

		static std::string k_packetizationMode = "packetizationMode";

		// MIME must match.
		if (this->mime != codec.mime)
			return false;

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
				if (!this->parameters.IncludesInteger(k_packetizationMode,
					codec.parameters.GetInteger(k_packetizationMode)))
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

	// NOTE: This method assumes that MatchesCodec() has been called before
	// with same codec and returned true.
	void RtpCodecCapability::Reduce(RtpCodecParameters& codec)
	{
		MS_TRACE();

		static std::string k_packetizationMode = "packetizationMode";

		switch (codec.mime.subtype)
		{
			case RTC::RtpCodecMime::Subtype::H264:
			{
				std::vector<int32_t> packetizationMode;

				// NOTE: `packetizationMode` is set by RtpCodecParameter if not present.
				packetizationMode.push_back(codec.parameters.GetInteger(k_packetizationMode));
				this->parameters.SetArrayOfIntegers(k_packetizationMode, packetizationMode);

				break;
			}

			default:
				;
		}
	}

	inline
	void RtpCodecCapability::CheckCodecCapability()
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

				std::vector<int32_t> packetizationMode = { 0 };

				if (!this->parameters.HasArrayOfIntegers(k_packetizationMode))
					this->parameters.SetArrayOfIntegers(k_packetizationMode, packetizationMode);

				break;
			}

			default:
				;
		}
	}
}
