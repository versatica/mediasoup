#ifndef MS_RTC_RTP_PARAMETERS_H
#define MS_RTC_RTP_PARAMETERS_H

#include "common.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
	// Lazy declarations.
	class RtpCodecParameters;
	class RtpEncodingParameters;

	class RtpParameters
	{
	public:
		RtpParameters(Json::Value& data);
		RtpParameters(const RtpParameters* RtpParameters);
		virtual ~RtpParameters();

		Json::Value toJson();

	public:
		// TODO: not sure if 1 or 2 bytes or what.
		std::string                        muxId = "";
		std::vector<RtpCodecParameters>    codecs;
		std::vector<RtpEncodingParameters> encodings;
	};

	class RtpCodecParameters
	{
	public:
		enum class Kind
		{
			BOTH  = 0,
			AUDIO = 1,
			VIDEO = 2
		};

	public:
		RtpCodecParameters(Json::Value& data);
		virtual ~RtpCodecParameters();

		Json::Value toJson();

	public:
		std::string name;
		Kind        kind = Kind::BOTH;
		uint8_t     payloadType = 0;
		uint32_t    clockRate = 0;
		uint32_t    maxptime = 0;
		uint32_t    numChannels = 0;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters(Json::Value& data);
		virtual ~RtpEncodingParameters();

		Json::Value toJson();

	public:
		uint32_t ssrc = 0;
	};
}

#endif
