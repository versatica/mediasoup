#define MS_CLASS "RTC::RtpEncodingParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Class static data. */

	// clang-format off
	std::map<std::string, RTC::RtpEncodingParameters::SpatialLayer> RTC::RtpEncodingParameters::string2SpatialLayer =
		{
			{ "none",    RTC::RtpEncodingParameters::SpatialLayer::NONE    },
			{ "default", RTC::RtpEncodingParameters::SpatialLayer::DEFAULT },
			{ "low",     RTC::RtpEncodingParameters::SpatialLayer::LOW     },
			{ "medium",  RTC::RtpEncodingParameters::SpatialLayer::MEDIUM  },
			{ "high",    RTC::RtpEncodingParameters::SpatialLayer::HIGH    }
		};

	std::map<RTC::RtpEncodingParameters::SpatialLayer, std::string> RTC::RtpEncodingParameters::spatialLayer2String =
		{
			{ RTC::RtpEncodingParameters::SpatialLayer::NONE,    "none"    },
			{ RTC::RtpEncodingParameters::SpatialLayer::DEFAULT, "default" },
			{ RTC::RtpEncodingParameters::SpatialLayer::LOW,     "low"     },
			{ RTC::RtpEncodingParameters::SpatialLayer::MEDIUM,  "medium"  },
			{ RTC::RtpEncodingParameters::SpatialLayer::HIGH,    "high"    }
		};
	// clang-format on

	/* Instance methods. */

	RtpEncodingParameters::RtpEncodingParameters(Json::Value& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_ERROR("data is not an object");

		auto jsonSsrcIt = data.find("ssrc");
		auto jsonRidIt = data.find("rid");
		auto jsonCodecPayloadTypeIt = data.find("codecPayloadType");
		auto jsonRtxIt = data.find("rtx");
		auto jsonMaxBitrateIt = data.find("maxBitrate");
		auto jsonMaxFramerateIt = data.find("maxFramerate");
		auto jsonActiveIt = data.find("active");
		auto jsonActiveIt = data.find("active");
		auto jsonSpatialLayerIt = data.find("spatialLayer");

		// ssrc is optional.
		if (jsonSsrcIt != data.end() && jsonSsrcIt->is_number_unsigned())
			this->ssrc = jsonSsrcIt->get<uint32_t>();

		// rid is optional.
		if (jsonRidIt != data.end() && jsonRidIt->is_string())
			this->rid = jsonRidIt->get<std::string>();

		// codecPayloadType is optional.
		if (jsonCodecPayloadTypeIt != data.end() && jsonCodecPayloadTypeIt->is_number_unsigned())
		{
			this->codecPayloadType    = jsonCodecPayloadTypeIt->get<uint8_t>();
			this->hasCodecPayloadType = true;
		}

		// rtx is optional.
		// This may throw.
		if (jsonRtxIt != data.end() && jsonRtxIt->is_object())
		{
			this->rtx    = RtpRtxParameters(*jsonRtxIt);
			this->hasRtx = true;
		}

		// maxBitrate is optional.
		if (jsonMaxBitrateIt != data.end() && jsonMaxBitrateIt->is_number_unsigned())
			this->maxBitrate = jsonMaxBitrateIt->get<uint32_t>();

		// maxFramerate is optional.
		if (jsonMaxFramerateIt != data.end() && jsonMaxFramerateIt->is_number())
			this->maxFramerate = jsonMaxFramerateIt->get<double>();

		// active is optional.
		if (jsonActiveIt != data.end() && jsonActiveIt->is_boolean())
			this->active = jsonActiveIt->get<bool>();

		// spatialLayer is optional.
		if (jsonSpatialLayerIt != data.end() && jsonSpatialLayerIt->is_string())
		{
			std::string spatialLayerStr = jsonSpatialLayerIt->get<std::string>();

			if (string2SpatialLayer.find(spatialLayerStr) == string2SpatialLayer.end())
				MS_THROW_ERROR("unknown spatialLayer");

			this->spatialLayer = string2SpatialLayer[spatialLayerStr];

			if (
			  this->spatialLayer == RTC::RtpEncodingParameters::SpatialLayer::NONE ||
			  this->spatialLayer == RTC::RtpEncodingParameters::SpatialLayer::DEFAULT)
			{
				MS_THROW_ERROR("invalid spatialLayer");
			}
		}
	}

	void RtpEncodingParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add ssrc.
		if (this->ssrc != 0u)
			jsonObject["ssrc"] = this->ssrc;

		// Add rid.
		if (!this->rid.empty())
			jsonObject["rid"] = this->rid;

		// Add codecPayloadType.
		if (this->hasCodecPayloadType)
			jsonObject["codecPayloadType"] = this->codecPayloadType;

		// Add rtx.
		if (this->hasRtx)
			this->rtx.FillJson(jsonObject["rtx"]);

		// Add maxBitrate.
		if (this->maxBitrate != 0u)
			jsonObject["maxBitrate"] = this->maxBitrate;

		// Add maxFramerate.
		if (this->maxFramerate > 0)
			jsonObject["maxFramerate"] = this->maxFramerate;

		// Add active.
		jsonObject["active"] = this->active;

		// Add spatialLayer.
		jsonObject["spatialLayer"] = RtpEncodingParameters::spatialLayer2String[this->spatialLayer];
	}
} // namespace RTC
