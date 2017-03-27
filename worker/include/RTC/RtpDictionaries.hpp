#ifndef MS_RTC_RTP_DICTIONARIES_HPP
#define MS_RTC_RTP_DICTIONARIES_HPP

#include "common.hpp"
#include "RTC/Parameters.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <json/json.h>

namespace RTC
{
	enum class Scope : uint8_t
	{
		ROOM_CAPABILITY = 1,
		PEER_CAPABILITY,
		RECEIVE
	};

	class Media
	{
	public:
		enum class Kind : uint8_t
		{
			ALL = 0,
			AUDIO,
			VIDEO,
			DEPTH
		};

	public:
		static Kind GetKind(std::string& str);
		static Json::StaticString& GetJsonString(Kind kind);

	private:
		static std::unordered_map<std::string, Kind> string2Kind;
		static std::map<Kind, Json::StaticString> kind2Json;
	};

	class RtpCodecMime
	{
	public:
		enum class Type : uint8_t
		{
			UNSET = 0,
			AUDIO,
			VIDEO
		};

	public:
		enum class Subtype : uint16_t
		{
			UNSET = 0,
			// Audio codecs:
			OPUS = 100,
			PCMA,
			PCMU,
			ISAC,
			G722,
			ILBC,
			SILK,
			// Video codecs:
			VP8 = 200,
			VP9,
			H264,
			H265,
			// Complementary codecs:
			CN = 300,
			TELEPHONE_EVENT,
			// Feature codecs:
			RTX = 400,
			ULPFEC,
			FLEXFEC,
			RED
		};

	private:
		static std::unordered_map<std::string, Type> string2Type;
		static std::map<Type, std::string> type2String;
		static std::unordered_map<std::string, Subtype> string2Subtype;
		static std::map<Subtype, std::string> subtype2String;

	public:
		RtpCodecMime() {};

		bool operator==(const RtpCodecMime& other) const
		{
			return this->type == other.type && this->subtype == other.subtype;
		}

		bool operator!=(const RtpCodecMime& other) const
		{
			return !(*this == other);
		}

		void SetName(std::string& name);

		const std::string& GetName() const
		{
			return this->name;
		}

		bool IsMediaCodec() const
		{
			return this->subtype >= Subtype(100) && this->subtype < (Subtype)300;
		}

		bool IsComplementaryCodec() const
		{
			return this->subtype >= Subtype(300) && this->subtype < (Subtype)400;
		}

		bool IsFeatureCodec() const
		{
			return this->subtype >= Subtype(400);
		}

	public:
		Type        type = Type::UNSET;
		Subtype     subtype = Subtype::UNSET;
		std::string name;
	};

	class RtpHeaderExtensionUri
	{
	public:
		enum class Type : uint8_t
		{
			UNKNOWN           = 0,
			SSRC_AUDIO_LEVEL  = 1,
			TO_OFFSET         = 2,
			ABS_SEND_TIME     = 3,
			VIDEO_ORIENTATION = 4,
			RTP_STREAM_ID     = 5
		};

		private:
			static std::unordered_map<std::string, Type> string2Type;

		public:
			static Type GetType(std::string& uri);
	};

	class RtcpFeedback
	{
	public:
		explicit RtcpFeedback(Json::Value& data);

		Json::Value toJson() const;

	public:
		std::string type;
		std::string parameter;
	};

	class RtpCodecParameters
	{
	public:
		RtpCodecParameters() {};
		RtpCodecParameters(Json::Value& data, RTC::Scope scope);

		Json::Value toJson() const;
		bool Matches(RtpCodecParameters& codec, bool checkPayloadType = false);
		void ReduceRtcpFeedback(std::vector<RtcpFeedback>& supportedRtcpFeedback);

	private:
		void CheckCodec();

	private:
		RTC::Scope                scope = RTC::Scope::ROOM_CAPABILITY;

	public:
		Media::Kind               kind = Media::Kind::ALL;
		RtpCodecMime              mime;
		uint8_t                   payloadType = 0;
		bool                      hasPayloadType = false;
		uint32_t                  clockRate = 0;
		uint32_t                  maxptime = 0;
		uint32_t                  ptime = 0;
		uint32_t                  numChannels = 1;
		RTC::Parameters           parameters;
		std::vector<RtcpFeedback> rtcpFeedback;
	};

	class RtpHeaderExtension
	{
	public:
		explicit RtpHeaderExtension(Json::Value& data);

		Json::Value toJson() const;

	public:
		Media::Kind                 kind = Media::Kind::ALL;
		std::string                 uri;
		RtpHeaderExtensionUri::Type type;
		uint8_t                     preferredId = 0;
		bool                        preferredEncrypt = false;
	};

	class RtpCapabilities
	{
	public:
		RtpCapabilities() {};
		RtpCapabilities(Json::Value& data, RTC::Scope scope);

		Json::Value toJson() const;
		void ReduceHeaderExtensions(std::vector<RtpHeaderExtension>& supportedHeaderExtensions);
		void ReduceFecMechanisms(std::vector<std::string>& supportedFecMechanisms);

	private:
		void ValidateCodecs(RTC::Scope scope);

	public:
		std::vector<RtpCodecParameters> codecs;
		std::vector<RtpHeaderExtension> headerExtensions;
		std::vector<std::string>        fecMechanisms;
	};

	class RtpFecParameters
	{
	public:
		RtpFecParameters() {};
		explicit RtpFecParameters(Json::Value& data);

		Json::Value toJson() const;

	public:
		std::string mechanism;
		uint32_t    ssrc = 0;
	};

	class RtpRtxParameters
	{
	public:
		RtpRtxParameters() {};
		explicit RtpRtxParameters(Json::Value& data);

		Json::Value toJson() const;

	public:
		uint32_t ssrc = 0;
	};

	class RtpEncodingParameters
	{
	public:
		RtpEncodingParameters() {};
		explicit RtpEncodingParameters(Json::Value& data);

		Json::Value toJson() const;

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
		explicit RtpHeaderExtensionParameters(Json::Value& data);

		Json::Value toJson() const;

	public:
		std::string                 uri;
		RtpHeaderExtensionUri::Type type;
		uint8_t                     id = 0;
		bool                        encrypt = false;
		RTC::Parameters             parameters;
	};

	class RtcpParameters
	{
	public:
		RtcpParameters() {};
		explicit RtcpParameters(Json::Value& data);

		Json::Value toJson() const;

	public:
		std::string cname;
		uint32_t    ssrc = 0;
		bool        reducedSize = true;
	};

	class RtpParameters
	{
	public:
		explicit RtpParameters(Json::Value& data);
		explicit RtpParameters(const RtpParameters* RtpParameters);

		Json::Value toJson() const;
		void ReduceCodecsAndEncodings(RtpCapabilities& capabilities);
		void ReduceHeaderExtensions(std::vector<RtpHeaderExtension>& supportedHeaderExtensions);
		RTC::RtpCodecParameters& GetCodecForEncoding(RtpEncodingParameters& encoding);

	private:
		void ValidateCodecs();
		void ValidateEncodings();

	public:
		std::string                               muxId;
		std::vector<RtpCodecParameters>           codecs;
		std::vector<RtpEncodingParameters>        encodings;
		std::vector<RtpHeaderExtensionParameters> headerExtensions;
		RtcpParameters                            rtcp;
		bool                                      hasRtcp = false;
		Json::Value                               userParameters;
	};
}

#endif
