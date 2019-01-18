#define MS_CLASS "RTC::RtpParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_TYPE_ERROR("data is not an object");

		auto jsonMidIt              = data.find("mid");
		auto jsonCodecsIt           = data.find("codecs");
		auto jsonEncodingsIt        = data.find("encodings");
		auto jsonHeaderExtensionsIt = data.find("headerExtensions");
		auto jsonRtcpIt             = data.find("rtcp");

		// mid is optional.
		if (jsonMidIt != data.end() && jsonMidIt->is_string())
		{
			this->mid = jsonMidIt->get<std::string>();

			if (this->mid.empty())
				MS_THROW_TYPE_ERROR("empty mid");
		}

		// codecs is mandatory.
		if (jsonCodecsIt == data.end() || !jsonCodecsIt->is_array())
			MS_THROW_TYPE_ERROR("missing codecs");

		for (auto& entry : *jsonCodecsIt)
		{
			// This may throw.
			RTC::RtpCodecParameters codec(entry);

			// Append to the codecs vector.
			this->codecs.push_back(codec);
		}

		if (this->codecs.empty())
			MS_THROW_TYPE_ERROR("empty codecs");

		// encodings is mandatory.
		if (jsonEncodingsIt == data.end() || !jsonEncodingsIt->is_array())
			MS_THROW_TYPE_ERROR("missing encodings");

		for (auto& entry : *jsonEncodingsIt)
		{
			// This may throw.
			RTC::RtpEncodingParameters encoding(entry);

			// Append to the encodings vector.
			this->encodings.push_back(encoding);
		}

		if (this->encodings.empty())
			MS_THROW_TYPE_ERROR("empty encodings");

		// headerExtensions is optional.
		if (jsonHeaderExtensionsIt != data.end() && jsonHeaderExtensionsIt->is_array())
		{
			for (auto& entry : *jsonHeaderExtensionsIt)
			{
				// This may throw.
				RTC::RtpHeaderExtensionParameters headerExtension(entry);

				// If a known header extension, append to the headerExtensions vector.
				if (headerExtension.type != RtpHeaderExtensionUri::Type::UNKNOWN)
					this->headerExtensions.push_back(headerExtension);
			}
		}

		// rtcp is optional.
		if (jsonRtcpIt != data.end() && jsonRtcpIt->is_object())
		{
			// This may throw.
			this->rtcp    = RTC::RtcpParameters(*jsonRtcpIt);
			this->hasRtcp = true;
		}

		// Validate RTP parameters.
		ValidateCodecs();
		ValidateEncodings();
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	  : mid(rtpParameters->mid), codecs(rtpParameters->codecs), encodings(rtpParameters->encodings),
	    headerExtensions(rtpParameters->headerExtensions), rtcp(rtpParameters->rtcp),
	    hasRtcp(rtpParameters->hasRtcp)
	{
		MS_TRACE();
	}

	void RtpParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add mid.
		if (!this->mid.empty())
			jsonObject["mid"] = this->mid;

		// Add codecs.
		jsonObject["codecs"] = json::array();
		auto jsonCodecsIt    = jsonObject.find("codecs");

		for (size_t i = 0; i < this->codecs.size(); ++i)
		{
			jsonCodecsIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonCodecsIt)[i];
			auto& codec     = this->codecs[i];

			codec.FillJson(jsonEntry);
		}

		// Add encodings.
		jsonObject["encodings"] = json::array();
		auto jsonEncodingsIt    = jsonObject.find("encodings");

		for (size_t i = 0; i < this->encodings.size(); ++i)
		{
			jsonEncodingsIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonEncodingsIt)[i];
			auto& encoding  = this->encodings[i];

			encoding.FillJson(jsonEntry);
		}

		// Add headerExtensions.
		jsonObject["headerExtensions"] = json::array();
		auto jsonHeaderExtensionsIt    = jsonObject.find("headerExtensions");

		for (size_t i = 0; i < this->headerExtensions.size(); ++i)
		{
			jsonHeaderExtensionsIt->emplace_back(json::value_t::object);

			auto& jsonEntry       = (*jsonHeaderExtensionsIt)[i];
			auto& headerExtension = this->headerExtensions[i];

			headerExtension.FillJson(jsonEntry);
		}

		// Add rtcp.
		if (this->hasRtcp)
			this->rtcp.FillJson(jsonObject["rtcp"]);
		else
			jsonObject["rtcp"] = json::object();
	}

	RTC::RtpCodecParameters& RtpParameters::GetCodecForEncoding(RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		static RTC::RtpCodecParameters fakeCodec;

		uint8_t payloadType = encoding.codecPayloadType;
		auto it             = this->codecs.begin();

		for (; it != this->codecs.end(); ++it)
		{
			auto& codec = *it;

			if (codec.payloadType == payloadType)
				return codec;
		}

		// This should never happen.
		if (it == this->codecs.end())
			MS_ABORT("no valid codec payload type for the given encoding");

		return fakeCodec;
	}

	RTC::RtpCodecParameters& RtpParameters::GetRtxCodecForEncoding(RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		static const std::string aptString = "apt";
		static RTC::RtpCodecParameters fakeCodec;

		uint8_t payloadType = encoding.codecPayloadType;

		for (auto it = this->codecs.begin(); it != this->codecs.end(); ++it)
		{
			auto& codec = *it;

			if (codec.mimeType.IsFeatureCodec() && codec.parameters.GetInteger(aptString) == payloadType)
			{
				return codec;
			}
		}

		return fakeCodec;
	}

	inline void RtpParameters::ValidateCodecs()
	{
		MS_TRACE();

		static std::string aptString{ "apt" };

		std::unordered_set<uint8_t> payloadTypes;

		for (auto& codec : this->codecs)
		{
			if (payloadTypes.find(codec.payloadType) != payloadTypes.end())
				MS_THROW_TYPE_ERROR("duplicated payloadType");

			payloadTypes.insert(codec.payloadType);

			switch (codec.mimeType.subtype)
			{
				// A RTX codec must have 'apt' parameter pointing to a non RTX codec.
				case RTC::RtpCodecMimeType::Subtype::RTX:
				{
					// NOTE: RtpCodecParameters already asserted that there is 'apt' parameter.
					int32_t apt = codec.parameters.GetInteger(aptString);
					auto it     = this->codecs.begin();

					for (; it != this->codecs.end(); ++it)
					{
						auto codec = *it;

						if (static_cast<int32_t>(codec.payloadType) == apt)
						{
							if (codec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::RTX)
								MS_THROW_TYPE_ERROR("apt in RTX codec points to a RTX codec");
							else if (codec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::ULPFEC)
								MS_THROW_TYPE_ERROR("apt in RTX codec points to a ULPFEC codec");
							else if (codec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::FLEXFEC)
								MS_THROW_TYPE_ERROR("apt in RTX codec points to a FLEXFEC codec");
							else
								break;
						}
					}

					if (it == this->codecs.end())
						MS_THROW_TYPE_ERROR("apt in RTX codec points to a non existing codec");

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
				if (codec.mimeType.IsMediaCodec())
				{
					firstMediaPayloadType = codec.payloadType;

					break;
				}
			}

			if (it == this->codecs.end())
				MS_THROW_TYPE_ERROR("no media codecs found");
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
						if (codec.mimeType.IsMediaCodec())
							break;

						MS_THROW_TYPE_ERROR("invalid codecPayloadType");
					}
				}

				if (it == this->codecs.end())
					MS_THROW_TYPE_ERROR("unknown codecPayloadType");
			}
		}
	}
} // namespace RTC
