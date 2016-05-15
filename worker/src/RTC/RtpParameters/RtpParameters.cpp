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
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_rtcp("rtcp");
		static const Json::StaticString k_userParameters("userParameters");

		// `muxId` is optional.
		if (data[k_muxId].isString())
			this->muxId = data[k_muxId].asString();

		// `codecs` is mandatory.
		if (data[k_codecs].isArray())
		{
			auto& json_codecs = data[k_codecs];

			if (json_codecs.size() == 0)
				MS_THROW_ERROR("empty `RtpCodecParameters.codecs`");

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
			{
				RtpCodecParameters codec(json_codecs[i]);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}
		else
		{
			MS_THROW_ERROR("missing `RtpCodecParameters.codecs`");
		}

		// `encodings` is optional.
		if (data[k_encodings].isArray())
		{
			auto& json_array = data[k_encodings];

			for (Json::UInt i = 0; i < json_array.size(); i++)
			{
				RtpEncodingParameters encoding(json_array[i]);

				// Append to the encodings vector.
				this->encodings.push_back(encoding);
			}
		}

		// `headerExtensions` is optional.
		if (data[k_headerExtensions].isArray())
		{
			auto& json_array = data[k_headerExtensions];

			for (Json::UInt i = 0; i < json_array.size(); i++)
			{
				RtpHeaderExtensionParameters headerExtension(json_array[i]);

				// Append to the headerExtensions vector.
				this->headerExtensions.push_back(headerExtension);
			}
		}

		// `rtcp` is optional.
		if (data[k_rtcp].isObject())
		{
			this->rtcp = RtcpParameters(data[k_rtcp]);
			this->hasRtcp = true;
		}

		// `userParameters` is optional.
		if (data[k_userParameters].isObject())
			this->userParameters = data[k_userParameters];
		else
			this->userParameters = Json::objectValue;
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	{
		MS_TRACE();

		this->muxId = rtpParameters->muxId;
		this->codecs = rtpParameters->codecs;
		this->encodings = rtpParameters->encodings;
		this->headerExtensions = rtpParameters->headerExtensions;
		this->rtcp = rtpParameters->rtcp;
		this->hasRtcp = rtpParameters->hasRtcp;
		this->userParameters = rtpParameters->userParameters;
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
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_rtcp("rtcp");
		static const Json::StaticString k_userParameters("userParameters");

		Json::Value json(Json::objectValue);

		// Add `muxId`.
		if (!this->muxId.empty())
			json[k_muxId] = this->muxId;

		// Add `codecs`.
		if (!this->codecs.empty())
		{
			json[k_codecs] = Json::arrayValue;

			for (auto& entry : this->codecs)
			{
				json[k_codecs].append(entry.toJson());
			}
		}

		// Add `encodings`.
		if (!this->encodings.empty())
		{
			json[k_encodings] = Json::arrayValue;

			for (auto& entry : this->encodings)
			{
				json[k_encodings].append(entry.toJson());
			}
		}

		// Add `headerExtensions`.
		if (!this->headerExtensions.empty())
		{
			json[k_headerExtensions] = Json::arrayValue;

			for (auto& entry : this->headerExtensions)
			{
				json[k_headerExtensions].append(entry.toJson());
			}
		}

		// Add `rtcp`.
		if (this->hasRtcp)
			json[k_rtcp] = this->rtcp.toJson();

		// Add `userParameters`.
		json[k_userParameters] = this->userParameters;

		return json;
	}
}
