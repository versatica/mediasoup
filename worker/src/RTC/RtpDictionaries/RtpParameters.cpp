#define MS_CLASS "RTC::RtpParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringMuxId{ "muxId" };
		static const Json::StaticString JsonStringCodecs{ "codecs" };
		static const Json::StaticString JsonStringEncodings{ "encodings" };
		static const Json::StaticString JsonStringHeaderExtensions{ "headerExtensions" };
		static const Json::StaticString JsonStringRtcp{ "rtcp" };

		// muxId is optional.
		if (data[JsonStringMuxId].isString())
		{
			this->muxId = data[JsonStringMuxId].asString();

			if (this->muxId.empty())
				MS_THROW_ERROR("empty rtpParameters.muxId");
		}

		// codecs is mandatory.
		if (!data[JsonStringCodecs].isArray())
			MS_THROW_ERROR("missing rtpParameters.codecs");

		auto& jsonCodecs = data[JsonStringCodecs];

		if (jsonCodecs.size() == 0)
			MS_THROW_ERROR("empty rtpParameters.codecs");

		for (auto& jsonCodec : jsonCodecs)
		{
			RTC::RtpCodecParameters codec(jsonCodec);

			// Append to the codecs vector.
			this->codecs.push_back(codec);
		}

		// encodings is mandatory.
		if (!data[JsonStringEncodings].isArray())
			MS_THROW_ERROR("missing rtpParameters.encodings");

		auto& jsonEncodings = data[JsonStringEncodings];

		if (jsonEncodings.size() == 0)
			MS_THROW_ERROR("empty rtpParameters.encodings");

		for (auto& i : jsonEncodings)
		{
			RTC::RtpEncodingParameters encoding(i);

			// Append to the encodings vector.
			this->encodings.push_back(encoding);
		}

		// headerExtensions is optional.
		if (data[JsonStringHeaderExtensions].isArray())
		{
			auto& jsonArray = data[JsonStringHeaderExtensions];

			for (auto& i : jsonArray)
			{
				RTC::RtpHeaderExtensionParameters headerExtension(i);

				// If a known header extension, append to the headerExtensions vector.
				if (headerExtension.type != RtpHeaderExtensionUri::Type::UNKNOWN)
					this->headerExtensions.push_back(headerExtension);
			}
		}

		// rtcp is optional.
		if (data[JsonStringRtcp].isObject())
		{
			this->rtcp    = RTC::RtcpParameters(data[JsonStringRtcp]);
			this->hasRtcp = true;
		}

		// Validate RTP parameters.
		ValidateCodecs();
		ValidateEncodings();
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	  : muxId(rtpParameters->muxId), codecs(rtpParameters->codecs),
	    encodings(rtpParameters->encodings), headerExtensions(rtpParameters->headerExtensions),
	    rtcp(rtpParameters->rtcp), hasRtcp(rtpParameters->hasRtcp)
	{
		MS_TRACE();
	}

	Json::Value RtpParameters::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringMuxId{ "muxId" };
		static const Json::StaticString JsonStringCodecs{ "codecs" };
		static const Json::StaticString JsonStringEncodings{ "encodings" };
		static const Json::StaticString JsonStringHeaderExtensions{ "headerExtensions" };
		static const Json::StaticString JsonStringRtcp{ "rtcp" };

		Json::Value json(Json::objectValue);

		// Add muxId.
		if (!this->muxId.empty())
			json[JsonStringMuxId] = this->muxId;

		// Add codecs.
		json[JsonStringCodecs] = Json::arrayValue;

		for (auto& entry : this->codecs)
		{
			json[JsonStringCodecs].append(entry.ToJson());
		}

		// Add encodings.
		json[JsonStringEncodings] = Json::arrayValue;

		for (auto& entry : this->encodings)
		{
			json[JsonStringEncodings].append(entry.ToJson());
		}

		// Add headerExtensions.
		json[JsonStringHeaderExtensions] = Json::arrayValue;

		for (auto& entry : this->headerExtensions)
		{
			json[JsonStringHeaderExtensions].append(entry.ToJson());
		}

		// Add rtcp.
		if (this->hasRtcp)
			json[JsonStringRtcp] = this->rtcp.ToJson();

		return json;
	}

	RTC::RtpCodecParameters& RtpParameters::GetCodecForEncoding(RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		static RTC::RtpCodecParameters fakeCodec;

		uint8_t payloadType = encoding.codecPayloadType;

		auto it = this->codecs.begin();
		for (; it != this->codecs.end(); ++it)
		{
			auto& codec = *it;

			if (codec.payloadType == payloadType)
				return codec;
		}
		// This should never happen.
		if (it == this->codecs.end())
		{
			MS_ABORT("no valid codec payload type for the given encoding");
		}

		return fakeCodec;
	}

	RTC::RtpCodecParameters& RtpParameters::GetRtxCodecForEncoding(RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		static const std::string associatedPayloadType = "apt";
		static RTC::RtpCodecParameters fakeCodec;

		uint8_t payloadType = encoding.codecPayloadType;

		auto it = this->codecs.begin();
		for (; it != this->codecs.end(); ++it)
		{
			auto& codec = *it;

			if (codec.mime.IsFeatureCodec() && codec.parameters.GetInteger(associatedPayloadType) == payloadType)
			{
				return codec;
			}
		}

		return fakeCodec;
	}

	inline void RtpParameters::ValidateCodecs()
	{
		MS_TRACE();

		static std::string jsonStringApt{ "apt" };

		std::unordered_set<uint8_t> payloadTypes;

		for (auto& codec : this->codecs)
		{
			if (payloadTypes.find(codec.payloadType) != payloadTypes.end())
				MS_THROW_ERROR("duplicated codec.payloadType");
			else
				payloadTypes.insert(codec.payloadType);

			switch (codec.mime.subtype)
			{
				// A RTX codec must have 'apt' parameter pointing to a non RTX codec.
				case RTC::RtpCodecMimeType::Subtype::RTX:
				{
					// NOTE: RtpCodecParameters already asserted that there is 'apt' parameter.
					int32_t apt = codec.parameters.GetInteger(jsonStringApt);
					auto it     = this->codecs.begin();

					for (; it != this->codecs.end(); ++it)
					{
						auto codec = *it;

						if (static_cast<int32_t>(codec.payloadType) == apt)
						{
							if (codec.mime.subtype == RTC::RtpCodecMimeType::Subtype::RTX)
								MS_THROW_ERROR("apt in RTX codec points to a RTX codec");
							else if (codec.mime.subtype == RTC::RtpCodecMimeType::Subtype::ULPFEC)
								MS_THROW_ERROR("apt in RTX codec points to a ULPFEC codec");
							else if (codec.mime.subtype == RTC::RtpCodecMimeType::Subtype::FLEXFEC)
								MS_THROW_ERROR("apt in RTX codec points to a FLEXFEC codec");
							else
								break;
						}
						if (it == this->codecs.end())
							MS_THROW_ERROR("apt in RTX codec points to a non existing codec");
					}

					break;
				}

				default:;
			}
		}
	}

	inline void RtpParameters::ValidateEncodings()
	{
		uint8_t firstMediaPayloadType = 0;

		{
			auto it = this->codecs.begin();

			for (; it != this->codecs.end(); ++it)
			{
				auto& codec = *it;

				// Must be a media codec.
				if (codec.mime.IsMediaCodec())
				{
					firstMediaPayloadType = codec.payloadType;

					break;
				}
			}
			if (it == this->codecs.end())
				MS_THROW_ERROR("no media codecs found");
		}

		// Iterate all the encodings, set the first payloadType in all of them with
		// codecPayloadType unset, and check that others point to a media codec.
		for (auto& encoding : this->encodings)
		{
			if (!encoding.hasCodecPayloadType)
			{
				encoding.codecPayloadType    = firstMediaPayloadType;
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
						if (codec.mime.IsMediaCodec())
							break;

						MS_THROW_ERROR("invalid encoding.codecPayloadType");
					}
				}
				if (it == this->codecs.end())
					MS_THROW_ERROR("unknown encoding.codecPayloadType");
			}
		}
	}
} // namespace RTC
