#define MS_CLASS "RTC::RtpParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RTP/Codecs/Tools.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Class variables. */

	// clang-format off
	// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
	const ankerl::unordered_dense::map<RtpParameters::Type, std::string> RtpParameters::Type2String = {
		{ RtpParameters::Type::SIMPLE,    "simple"    },
		{ RtpParameters::Type::SIMULCAST, "simulcast" },
		{ RtpParameters::Type::SVC,       "svc"       },
		{ RtpParameters::Type::PIPE,      "pipe"      }
	};
	// clang-format on

	/* Class methods. */

	std::optional<RtpParameters::Type> RtpParameters::GetType(const RtpParameters& rtpParameters)
	{
		MS_TRACE();

		std::optional<RtpParameters::Type> type;

		if (rtpParameters.encodings.size() == 1)
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			const auto& encoding         = rtpParameters.encodings[0];
			const auto* const mediaCodec = rtpParameters.GetCodecForEncoding(encoding);

			if (encoding.spatialLayers > 1 || encoding.temporalLayers > 1)
			{
				if (RTC::RTP::Codecs::Tools::IsValidTypeForCodec(RtpParameters::Type::SVC, mediaCodec->mimeType))
				{
					type.emplace(RtpParameters::Type::SVC);
				}
				else if (
				  RTC::RTP::Codecs::Tools::IsValidTypeForCodec(
				    RtpParameters::Type::SIMULCAST, mediaCodec->mimeType))
				{
					type.emplace(RtpParameters::Type::SIMULCAST);
				}
			}
			else
			{
				type.emplace(RtpParameters::Type::SIMPLE);
			}
		}
		else if (rtpParameters.encodings.size() > 1)
		{
			type.emplace(RtpParameters::Type::SIMULCAST);
		}

		return type;
	}

	const std::string& RtpParameters::GetTypeString(RtpParameters::Type type)
	{
		MS_TRACE();

		return RtpParameters::Type2String.at(type);
	}

	FBS::RtpParameters::Type RtpParameters::TypeToFbs(RtpParameters::Type type)
	{
		MS_TRACE();

		switch (type)
		{
			case Type::SIMPLE:
			{
				return FBS::RtpParameters::Type::SIMPLE;
			}

			case Type::SIMULCAST:
			{
				return FBS::RtpParameters::Type::SIMULCAST;
			}

			case Type::SVC:
			{
				return FBS::RtpParameters::Type::SVC;
			}

			case Type::PIPE:
			{
				return FBS::RtpParameters::Type::PIPE;
			}

				NO_DEFAULT();
		}
	}

	/* Instance methods. */

	RtpParameters::RtpParameters(const FBS::RtpParameters::RtpParameters* data)
	{
		MS_TRACE();

		// mid is optional.
		if (data->mid())
		{
			this->mid = data->mid()->str();
		}

		this->codecs.reserve(data->codecs()->size());

		for (const auto* entry : *data->codecs())
		{
			// This may throw due the constructor of RTC::RtpCodecParameters.
			this->codecs.emplace_back(entry);
		}

		if (this->codecs.empty())
		{
			MS_THROW_TYPE_ERROR("empty codecs");
		}

		this->encodings.reserve(data->encodings()->size());

		for (const auto* entry : *data->encodings())
		{
			// This may throw due the constructor of RTC::RtpEncodingParameters.
			this->encodings.emplace_back(entry);
		}

		if (this->encodings.empty())
		{
			MS_THROW_TYPE_ERROR("empty encodings");
		}

		this->headerExtensions.reserve(data->headerExtensions()->size());

		for (const auto* entry : *data->headerExtensions())
		{
			// This may throw due the constructor of RTC::RtpHeaderExtensionParameters.
			this->headerExtensions.emplace_back(entry);
		}

		// This may throw.
		this->rtcp = RTC::RtcpParameters(data->rtcp());

		// msid is optional.
		if (data->msid())
		{
			this->msid = data->msid()->str();
		}

		// Validate RTP parameters.
		ValidateCodecs();
		ValidateEncodings();
	}

	flatbuffers::Offset<FBS::RtpParameters::RtpParameters> RtpParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		// Add codecs.
		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters>> codecs;
		codecs.reserve(this->codecs.size());

		for (const auto& codec : this->codecs)
		{
			codecs.emplace_back(codec.FillBuffer(builder));
		}

		// Add encodings.
		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> encodings;
		encodings.reserve(this->encodings.size());

		for (const auto& encoding : this->encodings)
		{
			encodings.emplace_back(encoding.FillBuffer(builder));
		}

		// Add headerExtensions.
		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters>> headerExtensions;
		headerExtensions.reserve(this->headerExtensions.size());

		for (const auto& headerExtension : this->headerExtensions)
		{
			headerExtensions.emplace_back(headerExtension.FillBuffer(builder));
		}

		// Add rtcp.
		const auto rtcp = this->rtcp.FillBuffer(builder);

		return FBS::RtpParameters::CreateRtpParametersDirect(
		  builder,
		  !this->mid.empty() ? this->mid.c_str() : nullptr,
		  std::addressof(codecs),
		  std::addressof(headerExtensions),
		  std::addressof(encodings),
		  rtcp,
		  !this->msid.empty() ? this->msid.c_str() : nullptr);
	}

	const RTC::RtpCodecParameters* RtpParameters::GetCodecForEncoding(
	  const RtpEncodingParameters& encoding) const
	{
		MS_TRACE();

		// NOTE: ValidateEncodings() has already given every encoding a payload type.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const uint8_t payloadType = encoding.codecPayloadType.value();

		for (const auto& codec : this->codecs)
		{
			if (codec.payloadType == payloadType)
			{
				return std::addressof(codec);
			}
		}

		// This should never happen.
		MS_ABORT("no valid codec payload type for the given encoding");
	}

	const RTC::RtpCodecParameters* RtpParameters::GetRtxCodecForEncoding(
	  const RtpEncodingParameters& encoding) const
	{
		MS_TRACE();

		static const std::string AptString{ "apt" };

		// NOTE: ValidateEncodings() has already given every encoding a payload type.
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const uint8_t payloadType = encoding.codecPayloadType.value();

		for (const auto& codec : this->codecs)
		{
			// NOTE: Only RTX codecs are guaranteed to have an 'apt' parameter.
			if (codec.mimeType.subtype != RTC::RtpCodecMimeType::Subtype::RTX)
			{
				continue;
			}

			// NOTE: RtpCodecParameters already checked that 'apt' is a positive
			// integer.
			const auto apt = static_cast<uint32_t>(codec.parameters.GetInteger(AptString));

			if (apt == payloadType)
			{
				return std::addressof(codec);
			}
		}

		return nullptr;
	}

	void RtpParameters::ValidateCodecs()
	{
		MS_TRACE();

		static const std::string AptString{ "apt" };

		ankerl::unordered_dense::set<uint8_t> payloadTypes;

		for (const auto& codec : this->codecs)
		{
			const bool inserted = payloadTypes.insert(codec.payloadType).second;

			if (!inserted)
			{
				MS_THROW_TYPE_ERROR("duplicated payloadType");
			}

			// NOTE: We want a switch here, as in every other check per codec.
			// NOLINTNEXTLINE(readability-trivial-switch)
			switch (codec.mimeType.subtype)
			{
				// A RTX codec must have 'apt' parameter pointing to a non RTX codec.
				case RTC::RtpCodecMimeType::Subtype::RTX:
				{
					// NOTE: RtpCodecParameters already checked that 'apt' is a positive
					// integer.
					const auto apt = static_cast<uint32_t>(codec.parameters.GetInteger(AptString));

					const auto it = std::ranges::find_if(
					  this->codecs,
					  [apt](const RTC::RtpCodecParameters& otherCodec) -> bool
					  {
						  return otherCodec.payloadType == apt;
					  });

					if (it == this->codecs.end())
					{
						MS_THROW_TYPE_ERROR("apt in RTX codec points to a non existing codec");
					}

					const auto& otherCodec = *it;

					if (otherCodec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::RTX)
					{
						MS_THROW_TYPE_ERROR("apt in RTX codec points to a RTX codec");
					}
					else if (otherCodec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::ULPFEC)
					{
						MS_THROW_TYPE_ERROR("apt in RTX codec points to a ULPFEC codec");
					}
					else if (otherCodec.mimeType.subtype == RTC::RtpCodecMimeType::Subtype::FLEXFEC)
					{
						MS_THROW_TYPE_ERROR("apt in RTX codec points to a FLEXFEC codec");
					}

					break;
				}

				default:;
			}
		}
	}

	void RtpParameters::ValidateEncodings()
	{
		MS_TRACE();

		const auto firstMediaCodecIt = std::ranges::find_if(
		  this->codecs,
		  [](const RTC::RtpCodecParameters& codec) -> bool
		  {
			  return codec.mimeType.IsMediaCodec();
		  });

		if (firstMediaCodecIt == this->codecs.end())
		{
			MS_THROW_TYPE_ERROR("no media codecs found");
		}

		const auto& firstMediaCodec = *firstMediaCodecIt;

		// Iterate all the encodings, set the first payloadType in all of them with
		// codecPayloadType unset, and check that others point to a media codec.
		//
		// Also, don't allow multiple SVC spatial layers into an encoding if there
		// are more than one encoding (simulcast).
		for (auto& encoding : this->encodings)
		{
			if (encoding.spatialLayers > 1 && this->encodings.size() > 1)
			{
				MS_THROW_TYPE_ERROR(
				  "cannot use both simulcast and encodings with multiple SVC spatial layers");
			}

			if (!encoding.codecPayloadType.has_value())
			{
				encoding.codecPayloadType = firstMediaCodec.payloadType;
			}
			else
			{
				const auto it = std::ranges::find_if(
				  this->codecs,
				  [&encoding](const RTC::RtpCodecParameters& codec) -> bool
				  {
					  return codec.payloadType == encoding.codecPayloadType;
				  });

				if (it == this->codecs.end())
				{
					MS_THROW_TYPE_ERROR("unknown codecPayloadType");
				}

				const auto& codec = *it;

				// Must be a media codec.
				if (!codec.mimeType.IsMediaCodec())
				{
					MS_THROW_TYPE_ERROR("invalid codecPayloadType");
				}
			}
		}
	}
} // namespace RTC
