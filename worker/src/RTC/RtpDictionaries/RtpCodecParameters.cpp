#define MS_CLASS "RTC::RtpCodecParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(const FBS::RtpParameters::RtpCodecParameters* data)
	{
		MS_TRACE();

		// Set MIME field.
		// This may throw.
		this->mimeType.SetMimeType(data->mimeType()->str());

		// payloadType.
		this->payloadType = data->payloadType();

		// clockRate.
		this->clockRate = data->clockRate();

		// channels is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpCodecParameters::VT_CHANNELS))
		{
			this->channels = data->channels();
		}

		// parameters is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpCodecParameters::VT_PARAMETERS))
			this->parameters.Set(data->parameters());

		// rtcpFeedback is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpCodecParameters::VT_RTCPFEEDBACK))
		{
			this->rtcpFeedback.reserve(data->rtcpFeedback()->size());

			for (auto* entry : *data->rtcpFeedback())
			{
				this->rtcpFeedback.emplace_back(entry);
			}
		}

		// Check codec.
		CheckCodec();
	}

	void RtpCodecParameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add mimeType.
		jsonObject["mimeType"] = this->mimeType.ToString();

		// Add payloadType.
		jsonObject["payloadType"] = this->payloadType;

		// Add clockRate.
		jsonObject["clockRate"] = this->clockRate;

		// Add channels.
		if (this->channels > 1)
			jsonObject["channels"] = this->channels;

		// Add parameters.
		this->parameters.FillJson(jsonObject["parameters"]);

		// Add rtcpFeedback.
		jsonObject["rtcpFeedback"] = json::array();
		auto jsonRtcpFeedbackIt    = jsonObject.find("rtcpFeedback");

		for (size_t i{ 0 }; i < this->rtcpFeedback.size(); ++i)
		{
			jsonRtcpFeedbackIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonRtcpFeedbackIt)[i];
			auto& fb        = this->rtcpFeedback[i];

			fb.FillJson(jsonEntry);
		}
	}

	inline void RtpCodecParameters::CheckCodec()
	{
		MS_TRACE();

		static std::string aptString{ "apt" };

		// Check per MIME parameters and set default values.
		switch (this->mimeType.subtype)
		{
			case RTC::RtpCodecMimeType::Subtype::RTX:
			{
				// A RTX codec must have 'apt' parameter.
				if (!this->parameters.HasPositiveInteger(aptString))
					MS_THROW_TYPE_ERROR("missing apt parameter in RTX codec");

				break;
			}

			default:;
		}
	}
} // namespace RTC
