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

		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringCodecPayloadType{ "codecPayloadType" };
		static const Json::StaticString JsonStringFec{ "fec" };
		static const Json::StaticString JsonStringRtx{ "rtx" };
		static const Json::StaticString JsonStringResolutionScale{ "resolutionScale" };
		static const Json::StaticString JsonStringFramerateScale{ "framerateScale" };
		static const Json::StaticString JsonStringMaxFramerate{ "maxFramerate" };
		static const Json::StaticString JsonStringActive{ "active" };
		static const Json::StaticString JsonStringEncodingId{ "encodingId" };
		static const Json::StaticString JsonStringDependencyEncodingIds{ "dependencyEncodingIds" };

		if (!data.isObject())
			MS_THROW_ERROR("RtpEncodingParameters is not an object");

		// codecPayloadType is optional.
		if (data[JsonStringCodecPayloadType].isUInt())
		{
			this->codecPayloadType    = static_cast<uint8_t>(data[JsonStringCodecPayloadType].asUInt());
			this->hasCodecPayloadType = true;
		}

		// ssrc is optional.
		if (data[JsonStringSsrc].isUInt())
			this->ssrc = uint32_t{ data[JsonStringSsrc].asUInt() };

		// fec is optional.
		if (data[JsonStringFec].isObject())
		{
			this->fec    = RtpFecParameters(data[JsonStringFec]);
			this->hasFec = true;
		}

		// rtx is optional.
		if (data[JsonStringRtx].isObject())
		{
			this->rtx    = RtpRtxParameters(data[JsonStringRtx]);
			this->hasRtx = true;
		}

		// resolutionScale is optional.
		if (data[JsonStringResolutionScale].isDouble())
			this->resolutionScale = data[JsonStringResolutionScale].asDouble();

		// framerateScale is optional.
		if (data[JsonStringFramerateScale].isDouble())
			this->framerateScale = data[JsonStringFramerateScale].asDouble();

		// maxFramerate is optional.
		if (data[JsonStringMaxFramerate].isUInt())
			this->maxFramerate = uint32_t{ data[JsonStringMaxFramerate].asUInt() };

		// active is optional.
		if (data[JsonStringActive].isBool())
			this->active = data[JsonStringActive].asBool();

		// encodingId is optional.
		if (data[JsonStringEncodingId].isString())
			this->encodingId = data[JsonStringEncodingId].asString();

		// dependencyEncodingIds is optional.
		if (data[JsonStringDependencyEncodingIds].isArray())
		{
			auto& jsonArray = data[JsonStringDependencyEncodingIds];

			for (auto& entry : jsonArray)
			{
				// Append to the dependencyEncodingIds vector.
				if (entry.isString())
					this->dependencyEncodingIds.push_back(entry.asString());
			}
		}
	}

	Json::Value RtpEncodingParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringCodecPayloadType{ "codecPayloadType" };
		static const Json::StaticString JsonStringFec{ "fec" };
		static const Json::StaticString JsonStringRtx{ "rtx" };
		static const Json::StaticString JsonStringResolutionScale{ "resolutionScale" };
		static const Json::StaticString JsonStringFramerateScale{ "framerateScale" };
		static const Json::StaticString JsonStringMaxFramerate{ "maxFramerate" };
		static const Json::StaticString JsonStringActive{ "active" };
		static const Json::StaticString JsonStringEncodingId{ "encodingId" };
		static const Json::StaticString JsonStringDependencyEncodingIds{ "dependencyEncodingIds" };

		Json::Value json(Json::objectValue);

		// Add codecPayloadType.
		if (this->hasCodecPayloadType)
			json[JsonStringCodecPayloadType] = Json::UInt{ this->codecPayloadType };

		// Add ssrc.
		if (this->ssrc != 0u)
			json[JsonStringSsrc] = Json::UInt{ this->ssrc };

		// Add fec
		if (this->hasFec)
			json[JsonStringFec] = this->fec.ToJson();

		// Add rtx
		if (this->hasRtx)
			json[JsonStringRtx] = this->rtx.ToJson();

		// Add resolutionScale (if different than the default value).
		if (this->resolutionScale != 1.0)
			json[JsonStringResolutionScale] = this->resolutionScale;

		// Add framerateScale (if different than the default value).
		if (this->framerateScale != 1.0)
			json[JsonStringFramerateScale] = this->framerateScale;

		// Add maxFramerate.
		if (this->maxFramerate != 0u)
			json[JsonStringMaxFramerate] = Json::UInt{ this->maxFramerate };

		// Add active.
		json[JsonStringActive] = this->active;

		// Add encodingId.
		if (!this->encodingId.empty())
			json[JsonStringEncodingId] = this->encodingId;

		// Add `dependencyEncodingIds` (if any).
		if (!this->dependencyEncodingIds.empty())
		{
			json[JsonStringDependencyEncodingIds] = Json::arrayValue;

			for (auto& entry : this->dependencyEncodingIds)
			{
				json[JsonStringDependencyEncodingIds].append(entry);
			}
		}

		return json;
	}
} // namespace RTC
