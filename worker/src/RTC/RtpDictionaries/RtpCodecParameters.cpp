#define MS_CLASS "RTC::RtpCodecParameters"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpCodecParameters::RtpCodecParameters(json& data)
	{
		MS_TRACE();

		if (!data.is_object())
			MS_THROW_ERROR("data is not an object");

		auto jsonMimeTypeIt     = data.find("mimeType");
		auto jsonPayloadTypeIt  = data.find("payloadType");
		auto jsonClockRateIt    = data.find("clockRate");
		auto jsonChannelsIt     = data.find("channels");
		auto jsonParametersIt   = data.find("parameters");
		auto jsonRtcpFeedbackIt = data.find("rtcpFeedback");

		// mimeType is mandatory.
		if (jsonMimeTypeIt == data.end() || !jsonMimeTypeIt->is_string())
			MS_THROW_ERROR("missing mimeType");

		// Set MIME field.
		// NOTE: This may throw.
		this->mimeType.SetMimeType(jsonMimeTypeIt->get<std::string>());

		// payloadType is mandatory.
		if (jsonPayloadTypeIt == data.end() || !jsonPayloadTypeIt->is_number_unsigned())
			MS_THROW_ERROR("missing payloadType");

		this->payloadType = jsonPayloadTypeIt->get<uint8_t>();

		// clockRate is mandatory.
		if (jsonClockRateIt == data.end() || !jsonClockRateIt->is_number_unsigned())
			MS_THROW_ERROR("missing clockRate");

		this->clockRate = jsonClockRateIt->get<uint32_t>();

		// channels is optional.
		if (jsonChannelsIt != data.end() && jsonChannelsIt->is_number_unsigned())
			this->channels = jsonChannelsIt->get<uint8_t>();

		// parameters is optional.
		if (jsonParametersIt != data.end() && jsonParametersIt->is_object())
			this->parameters.Set(*jsonParametersIt);

		// rtcpFeedback is optional.
		if (jsonRtcpFeedbackIt != data.end() && jsonRtcpFeedbackIt->is_array())
		{
			for (auto& entry : *jsonRtcpFeedbackIt)
			{
				RTC::RtcpFeedback rtcpFeedback(entry);

				// Append to the rtcpFeedback vector.
				this->rtcpFeedback.push_back(rtcpFeedback);
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

		// Add name.
		jsonObject["name"] = this->mimeType.GetName();

		// Add payloadType.
		jsonObject["payloadType"] = this->payloadType;

		// Add clockRate.
		jsonObject["clockRate"] = this->clockRate;

		// Add channels.
		if (this->channels > 0)
			jsonObject["channels"] = this->channels;

		// Add parameters.
		this->parameters.FillJson(jsonObject["parameters"]);

		// Add rtcpFeedback.
		jsonObject["rtcpFeedback"] = json::array();
		auto jsonRtcpFeedbackIt    = jsonObject.find("rtcpFeedback");

		for (auto i = 0; i < this->rtcpFeedback.size(); ++i)
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
				if (!this->parameters.HasInteger(aptString))
					MS_THROW_ERROR("missing apt parameter in RTX codec");

				break;
			}

			default:;
		}
	}
} // namespace RTC
