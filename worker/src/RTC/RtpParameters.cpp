#define MS_CLASS "RTC::RtpParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_muxId("muxId");
		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_encodings("encodings");

		// `muxId` is optional.
		if (data[k_muxId].isString())
		{
			this->muxId = data[k_muxId].asString();
		}

		// `codecs` is mandatory.
		if (data[k_codecs].isArray())
		{
			auto& json_codecs = data[k_codecs];

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
			{
				RtpCodecParameters codec(json_codecs[i]);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}
		else
		{
			MS_THROW_ERROR("missing `codecs`");
		}

		// `encodings` is mandatory.
		if (data[k_encodings].isArray())
		{
			auto& json_encodings = data[k_encodings];

			for (Json::UInt i = 0; i < json_encodings.size(); i++)
			{
				RtpEncodingParameters encoding(json_encodings[i]);

				// Append to the encodings vector.
				this->encodings.push_back(encoding);
			}
		}
		else
		{
			MS_THROW_ERROR("missing `encodings`");
		}
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	{
		MS_TRACE();

		this->muxId = rtpParameters->muxId;
		this->codecs = rtpParameters->codecs;
		this->encodings = rtpParameters->encodings;
	}

	RtpParameters::~RtpParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_muxId("muxId");
		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_encodings("encodings");

		Json::Value json(Json::objectValue);

		// Add `muxId`.
		json[k_muxId] = this->muxId;

		// Add `codecs`.
		json[k_codecs] = Json::arrayValue;
		for (auto it = this->codecs.begin(); it != this->codecs.end(); ++it)
		{
			RtpCodecParameters* codec = std::addressof(*it);

			json[k_codecs].append(codec->toJson());
		}

		// Add `encodings`.
		json[k_encodings] = Json::arrayValue;
		for (auto it = this->encodings.begin(); it != this->encodings.end(); ++it)
		{
			RtpEncodingParameters* encoding = std::addressof(*it);

			json[k_encodings].append(encoding->toJson());
		}

		return json;
	}
}
