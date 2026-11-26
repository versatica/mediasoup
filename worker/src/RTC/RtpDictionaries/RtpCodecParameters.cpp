#define MS_CLASS "RTC::RtpCodecParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Static. */

	// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
	static const std::string AptString{ "apt" };

	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(const FBS::RtpParameters::RtpCodecParameters* data)
	  : payloadType(data->payloadType()), clockRate(data->clockRate())
	{
		MS_TRACE();

		// Set MIME field.
		// This may throw.
		this->mimeType.SetMimeType(data->mimeType()->str());

		// The payload type field of the RTP header is 7 bits long.
		if (this->payloadType > 127)
		{
			MS_THROW_TYPE_ERROR("invalid payloadType %" PRIu8 " (greater than 127)", this->payloadType);
		}

		// channels is optional.
		if (const auto channels = data->channels(); channels.has_value())
		{
			this->channels = channels.value();
		}

		// parameters is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpCodecParameters::VT_PARAMETERS))
		{
			this->parameters.Set(data->parameters());
		}

		// rtcpFeedback is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpCodecParameters::VT_RTCPFEEDBACK))
		{
			this->rtcpFeedback.reserve(data->rtcpFeedback()->size());

			for (const auto* entry : *data->rtcpFeedback())
			{
				this->rtcpFeedback.emplace_back(entry);
			}
		}

		// Check codec.
		CheckCodec();
	}

	flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters> RtpCodecParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		const auto parameters = this->parameters.FillBuffer(builder);

		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback>> rtcpFeedback;

		rtcpFeedback.reserve(this->rtcpFeedback.size());

		for (const auto& fb : this->rtcpFeedback)
		{
			rtcpFeedback.emplace_back(fb.FillBuffer(builder));
		}

		return FBS::RtpParameters::CreateRtpCodecParametersDirect(
		  builder,
		  this->mimeType.ToString().c_str(),
		  this->payloadType,
		  this->clockRate,
		  this->channels > 1 ? flatbuffers::Optional<uint8_t>(this->channels) : flatbuffers::nullopt,
		  std::addressof(parameters),
		  std::addressof(rtcpFeedback));
	}

	bool RtpCodecParameters::HasRtcpFeedbackType(std::string_view type) const
	{
		MS_TRACE();

		return std::ranges::any_of(
		  this->rtcpFeedback,
		  [type](const RTC::RtcpFeedback& fb) -> bool
		  {
			  return fb.type == type;
		  });
	}

	inline void RtpCodecParameters::CheckCodec() const
	{
		MS_TRACE();

		// Check per MIME parameters and set default values.
		// NOTE: We want a switch here, as in every other check per codec.
		// NOLINTNEXTLINE(readability-trivial-switch)
		switch (this->mimeType.subtype)
		{
			case RTC::RtpCodecMimeType::Subtype::RTX:
			{
				// A RTX codec must have 'apt' parameter.
				if (!this->parameters.HasPositiveInteger(AptString))
				{
					MS_THROW_TYPE_ERROR("missing apt parameter in RTX codec");
				}

				break;
			}

			default:;
		}
	}
} // namespace RTC
