#ifndef MS_RTC_RTP_PARAMETERS_H
#define MS_RTC_RTP_PARAMETERS_H

#include "common.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
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
		RtpCodecParameters(Json::Value& data);
		virtual ~RtpCodecParameters();

		Json::Value toJson();

	public:
		std::string               name;
		uint8_t                   payloadType = 0;
		uint32_t                  clockRate = 0;
		uint32_t                  maxptime = 0;
		uint32_t                  numChannels = 0;
		std::vector<RtcpFeedback> rtcpFeedback;
		// TODO: Dictionary parameters;
	};

	class RtpFecParameters
	{
	public:
		RtpFecParameters() {};
		RtpFecParameters(Json::Value& data);
		virtual ~RtpFecParameters();

		Json::Value toJson();

	public:
		std::string mechanism;
		uint32_t    ssrc = 0;
	};

	class RtpRtxParameters
	{
	public:
		RtpRtxParameters() {};
		RtpRtxParameters(Json::Value& data);
		virtual ~RtpRtxParameters();

		Json::Value toJson();

	public:
		uint8_t  payloadType = 0;
		uint32_t ssrc = 0;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters(Json::Value& data);
		virtual ~RtpEncodingParameters();

		Json::Value toJson();

	public:
		uint32_t         ssrc = 0;
		uint8_t          codecPayloadType = 0;
		bool             hasFec = false;
		RtpFecParameters fec;
		bool             hasRtx = false;
		RtpRtxParameters rtx;
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
		enum class Kind
		{
			AUDIO = 1,
			VIDEO = 2
		};

	public:
		RtpParameters(Json::Value& data);
		RtpParameters(const RtpParameters* RtpParameters);
		virtual ~RtpParameters();

		Json::Value toJson();

	public:
		Kind                               kind;
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
