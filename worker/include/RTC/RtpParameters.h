#ifndef MS_RTC_RTP_PARAMETERS_H
#define MS_RTC_RTP_PARAMETERS_H

#include "common.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class RtpParameters
	{
	public:
		enum class Kind
		{
			BOTH  = 0,
			AUDIO = 1,
			VIDEO = 2
		};

	public:
		struct CodecParameters
		{
			std::string name;
			Kind        kind = Kind::BOTH;
			uint8_t     payloadType;
			uint32_t    clockRate = 0;
			uint32_t    maxptime = 0;
			uint32_t    numChannels = 0;
			// sequence<RTCRtcpFeedback> rtcpFeedback;
			// Dictionary                parameters;
		};

	public:
		struct EncodingParameters
		{
			uint32_t ssrc = 0;
			uint8_t  codecPayloadType = 0;
			// [...]
		};

	public:
		RtpParameters(Json::Value& data);
		virtual ~RtpParameters();

		Json::Value toJson();

	private:
		void AddCodec(Json::Value& data);
		void AddEncoding(Json::Value& data);

	public:
		// TODO: not sure if 1 or 2 bytes or what...
		std::string muxId = "";
		std::vector<CodecParameters> codecs;
		std::vector<EncodingParameters> encodings;
		// TODO: more fields missing
	};
}

#endif
