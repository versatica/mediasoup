#ifndef MS_RTC_RTP_PARAMETERS_H
#define MS_RTC_RTP_PARAMETERS_H

#include "common.h"
#include "RTC/RtpKind.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class CustomParameterValue
	{
		public:
			enum class Type
			{
				BOOLEAN = 1,
				INTEGER,
				DOUBLE,
				STRING
			};

		public:
			CustomParameterValue() {};
			CustomParameterValue(bool booleanValue);
			CustomParameterValue(uint32_t integerValue);
			CustomParameterValue(double doubleValue);
			CustomParameterValue(std::string& stringValue);
			virtual ~CustomParameterValue();

			Json::Value toJson();

		public:
			Type        type;
			bool        booleanValue = false;
			uint32_t    integerValue = 0;
			double      doubleValue = 0.0;
			std::string stringValue;
	};

	typedef std::unordered_map<std::string, CustomParameterValue> CustomParameters;

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
		enum class Subtype
		{
			MEDIA = 1,
			RTX,
			ULPFEC,
			FLEXFEC,
			RED,
			CN,
			DTMF
		};

	private:
		static std::unordered_map<std::string, RtpCodecParameters::Subtype> string2Subtype;

	public:
		RtpCodecParameters(RTC::RtpKind kind, Json::Value& data);
		virtual ~RtpCodecParameters();

		Json::Value toJson();

	public:
		std::string               name;
		uint8_t                   payloadType = 0;
		uint32_t                  clockRate = 0;
		uint32_t                  maxptime = 0;
		uint32_t                  ptime = 0;
		uint32_t                  numChannels = 0;
		std::vector<RtcpFeedback> rtcpFeedback;
		CustomParameters          parameters;

	public:
		RTC::RtpKind              type;
		Subtype                   subtype;
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
		uint32_t ssrc = 0;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters(Json::Value& data);
		RtpEncodingParameters();
		virtual ~RtpEncodingParameters();

		Json::Value toJson();

	public:
		uint32_t                 ssrc = 0;
		uint8_t                  codecPayloadType = 0;
		bool                     hasCodecPayloadType = false;
		RtpFecParameters         fec;
		bool                     hasFec = false;
		RtpRtxParameters         rtx;
		bool                     hasRtx = false;
		double                   resolutionScale = 1.0;
		double                   framerateScale = 1.0;
		uint32_t                 maxFramerate = 0;
		bool                     active = true;
		std::string              encodingId;
		std::vector<std::string> dependencyEncodingIds;
	};

	class RtpHeaderExtensionParameters
	{
	public:
		RtpHeaderExtensionParameters() {};
		RtpHeaderExtensionParameters(Json::Value& data);
		virtual ~RtpHeaderExtensionParameters();

		Json::Value toJson();

	public:
		std::string      uri;
		uint16_t         id = 0;
		bool             encrypt = false;
		CustomParameters parameters;
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
		static RtpParameters* Factory(RTC::RtpKind kind, Json::Value& data);

	private:
		RtpParameters(RTC::RtpKind kind, Json::Value& data);

	public:
		RtpParameters(const RtpParameters* RtpParameters);
		virtual ~RtpParameters();

		Json::Value toJson();
		void AutoFill();

	public:
		// TODO: not sure if 1 or 2 bytes or what.
		std::string                        muxId;
		std::vector<RtpCodecParameters>    codecs;
		std::vector<RtpEncodingParameters> encodings;
		std::vector<RtpHeaderExtensionParameters> headerExtensions;
		RtcpParameters                     rtcp;
		bool                               hasRtcp = false;
		Json::Value                        userParameters;

	public:
		RTC::RtpKind                       kind;
	};
}

#endif
