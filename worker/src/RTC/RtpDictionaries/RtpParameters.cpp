#define MS_CLASS "RTC::RtpParameters"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(RTC::RtpKind kind, Json::Value& data) :
		kind(kind)
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
		{
			this->muxId = data[k_muxId].asString();

			if (this->muxId.empty())
				MS_THROW_ERROR("empty RtpParameters.muxId");
		}

		// `codecs` is mandatory.
		if (data[k_codecs].isArray())
		{
			auto& json_codecs = data[k_codecs];

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
			{
				RtpCodecParameters codec(this->kind, json_codecs[i]);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}
		else
		{
			MS_THROW_ERROR("missing RtpParameters.codecs");
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

		// Validate RTP parameters.

		ValidateCodecs();
		ValidateEncodings();
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	{
		MS_TRACE();

		this->kind = rtpParameters->kind;
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

	void RtpParameters::ValidateCodecs()
	{
		MS_TRACE();

		// Must be at least one codec.
		if (this->codecs.size() == 0)
			MS_THROW_ERROR("empty RtpParameters.codecs");

		std::unordered_set<uint8_t> payloadTypes;

		// Validate each codec.
		for (auto& codec : this->codecs)
		{
			// Payload types must be unique.
			if (payloadTypes.find(codec.payloadType) != payloadTypes.end())
				MS_THROW_ERROR("duplicated codec.payloadType");
			else
				payloadTypes.insert(codec.payloadType);

			switch (codec.subtype)
			{
				// A RTX codec must have 'apt' parameter pointing to a non RTX codec.
				case RtpCodecParameters::Subtype::RTX:
				{
					// NOTE: RtpCodecParameters already asserted that there is 'apt' parameter.
					auto it = codec.parameters.find("apt");
					int32_t apt = it->second.integerValue;
					auto it2 = this->codecs.begin();

					for (; it2 != this->codecs.end(); ++it2)
					{
						auto codec = *it2;

						if ((int32_t)codec.payloadType == apt)
						{
							if (codec.subtype == RtpCodecParameters::Subtype::RTX)
								MS_THROW_ERROR("apt in RTX codec points to a RTX codec");
							else if (codec.subtype == RtpCodecParameters::Subtype::ULPFEC)
								MS_THROW_ERROR("apt in RTX codec points to a ULPFEC codec");
							else if (codec.subtype == RtpCodecParameters::Subtype::FLEXFEC)
								MS_THROW_ERROR("apt in RTX codec points to a FLEXFEC codec");
							else
								break;
						}
					}
					if (it2 == this->codecs.end())
						MS_THROW_ERROR("apt in RTX codec points to a non existing codec");
				}

				default:
					;
			}
		}
	}

	void RtpParameters::ValidateEncodings()
	{
		uint8_t firstMediaPayloadType;
		bool hasFirstMediaPayloadType = false;

		{
			auto it = this->codecs.begin();

			for (; it != this->codecs.end(); ++it)
			{
				auto codec = *it;

				// Must be a media codec.
				if (codec.subtype == RtpCodecParameters::Subtype::MEDIA)
				{
					firstMediaPayloadType = codec.payloadType;
					hasFirstMediaPayloadType = true;

					break;
				}
			}
			if (it == this->codecs.end())
				MS_THROW_ERROR("no media codecs found");
		}

		// If there are no encodings create one with `codecPayloadType` pointing to
		// the first media codec.
		if (this->encodings.size() == 0)
		{
			RtpEncodingParameters encoding;

			encoding.codecPayloadType = firstMediaPayloadType;
			encoding.hasCodecPayloadType = true;

			// Insert into the encodings vector.
			this->encodings.push_back(encoding);
		}
		// Otherwise iterate all the encodings, set the first payloadType in all of them
		// with codecPayloadType unset, and check that others point to a media codec.
		else
		{
			for (auto& encoding : this->encodings)
			{
				if (!encoding.hasCodecPayloadType)
				{
					encoding.codecPayloadType = firstMediaPayloadType;
					encoding.hasCodecPayloadType = true;
				}
				else
				{
					auto it = this->codecs.begin();

					for (; it != this->codecs.end(); ++it)
					{
						auto codec = *it;

						if (codec.payloadType == encoding.codecPayloadType)
						{
							// Must be a media codec.
							if (codec.subtype == RtpCodecParameters::Subtype::MEDIA)
								break;
							else
								MS_THROW_ERROR("invalid encoding.codecPayloadType");
						}
					}
					if (it == this->codecs.end())
						MS_THROW_ERROR("unknown encoding.codecPayloadType");
				}
			}
		}
	}
}
