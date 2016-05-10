#ifndef MS_RTC_RTP_PARAMETERS_H
#define MS_RTC_RTP_PARAMETERS_H

#include "common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class RtxCodecParameters
	{
	public:
		RtxCodecParameters() {};
		RtxCodecParameters(Json::Value& data);
		virtual ~RtxCodecParameters();

		Json::Value toJson();

	public:
		uint8_t  payloadType = 0;
		uint32_t rtxTime = 0;
	};

	class FecCodecParameters
	{
	public:
		FecCodecParameters() {};
		FecCodecParameters(Json::Value& data);
		virtual ~FecCodecParameters();

		Json::Value toJson();

	public:
		std::string mechanism;
		uint8_t     payloadType = 0;
		// TODO: optional parameters
		// https://github.com/openpeer/ortc/issues/444#issuecomment-203560671
	};

	class RtcpFeedback
	{
	public:
		RtcpFeedback(Json::Value& data);
		virtual ~RtcpFeedback();

		Json::Value toJson();

	public:
		std::string type;
		std::string parameter;
	};

	class RtpCodecParameters
	{
	public:
		class ParameterValue
		{
			public:
				enum class Type
				{
					BOOLEAN = 1,
					STRING,
					INTEGER
				};

			public:
				ParameterValue() {};
				ParameterValue(bool booleanValue);
				ParameterValue(uint32_t integerValue);
				ParameterValue(std::string& stringValue);
				virtual ~ParameterValue();

				Json::Value toJson();

			public:
				Type        type;
				bool        booleanValue = false;
				uint32_t    integerValue = 0;
				std::string stringValue = "";
		};

	public:
		RtpCodecParameters(Json::Value& data);
		virtual ~RtpCodecParameters();

		Json::Value toJson();

	public:
		std::string                     name;
		uint8_t                         payloadType = 0;
		uint32_t                        clockRate = 0;
		uint32_t                        maxptime = 0;
		uint32_t                        numChannels = 0;
		bool                            hasRtx = false;
		RtxCodecParameters              rtx;
		std::vector<FecCodecParameters> fec;
		std::vector<RtcpFeedback>       rtcpFeedback;
		std::unordered_map<std::string, ParameterValue> parameters;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters(Json::Value& data);
		virtual ~RtpEncodingParameters();

		Json::Value toJson();

	public:
		uint8_t  codecPayloadType = 0;
		uint32_t ssrc = 0;
		uint32_t rtxSsrc = 0;
		uint32_t fecSsrc = 0;
	};

	class RtcpParameters
	{
	public:
		RtcpParameters() {};
		RtcpParameters(Json::Value& data);
		virtual ~RtcpParameters();

		Json::Value toJson();

	public:
		std::string cname;
		uint32_t    ssrc = 0;
		bool        reducedSize = false;
	};

	class RtpParameters
	{
	public:
		RtpParameters(Json::Value& data);
		RtpParameters(const RtpParameters* RtpParameters);
		virtual ~RtpParameters();

		Json::Value toJson();

	public:
		// TODO: not sure if 1 or 2 bytes or what.
		std::string                        muxId;
		std::vector<RtpCodecParameters>    codecs;
		// TODO:
		// std::vector<RtpHeaderExtensionParameters> headerExtensions;
		std::vector<RtpEncodingParameters> encodings;
		RtcpParameters                     rtcp;
		// TODO:
		// DegradationPreference              degradationPreference = "balanced";
	};
}

#endif
