#define MS_CLASS "RTC::RtpParameters"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_muxId("muxId");
		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_encodings("encodings");

		// `muxId` is optional.
		if (data[k_muxId].isString())
		{
			this->muxId = data[k_muxId].asString();
		}

		// `codecs` is mandatory.
		if (data[k_codecs].isArray())
		{
			auto json_codecs = data[k_codecs];

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
				AddCodec(json_codecs[i]);
		}
		else
		{
			MS_THROW_ERROR("missing `codecs`");
		}

		//  `encodings` is mandatory.
		if (data[k_encodings].isArray())
		{
			auto json_encodings = data[k_encodings];

			for (Json::UInt i = 0; i < json_encodings.size(); i++)
				AddEncoding(json_encodings[i]);
		}
		else
		{
			MS_THROW_ERROR("missing `encodings`");
		}
	}

	RtpParameters::RtpParameters(const RtpParameters* rtpParameters)
	{
		MS_TRACE();

		this->muxId = rtpParameters->muxId;
		this->codecs = rtpParameters->codecs;
		this->encodings = rtpParameters->encodings;
	}

	RtpParameters::~RtpParameters()
	{
		MS_TRACE();
	}

	Json::Value RtpParameters::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_muxId("muxId");
		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_name("name");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString v_both("");
		static const Json::StaticString v_audio("audio");
		static const Json::StaticString v_video("video");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_encodings("encodings");
		static const Json::StaticString k_ssrc("ssrc");

		Json::Value json(Json::objectValue);

		// Add `muxId`.
		json[k_muxId] = this->muxId;

		// Add `codecs`.
		json[k_codecs] = Json::arrayValue;
		for (auto it = this->codecs.begin(); it != this->codecs.end(); ++it)
		{
			CodecParameters* codec = std::addressof(*it);
			Json::Value json_codec;

			json_codec[k_name] = codec->name;

			switch(codec->kind)
			{
				case Kind::BOTH:
					json_codec[k_kind] = v_both;
					break;
				case Kind::AUDIO:
					json_codec[k_kind] = v_audio;
					break;
				case Kind::VIDEO:
					json_codec[k_kind] = v_video;
					break;
			}

			json_codec[k_payloadType] = (Json::UInt)codec->payloadType;

			if (codec->clockRate)
				json_codec[k_clockRate] = (Json::UInt)codec->clockRate;
			else
				json_codec[k_clockRate] = Json::nullValue;

			json[k_codecs].append(json_codec);
		}

		// Add `encodings`.
		json[k_encodings] = Json::arrayValue;
		for (auto it = this->encodings.begin(); it != this->encodings.end(); ++it)
		{
			EncodingParameters* encoding = std::addressof(*it);
			Json::Value json_encoding;

			json_encoding[k_ssrc] = (Json::UInt)encoding->ssrc;

			json[k_encodings].append(json_encoding);
		}

		return json;
	}

	void RtpParameters::AddCodec(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_name("name");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");

		if (!data.isObject())
			MS_THROW_ERROR("data is not an object");

		struct CodecParameters codec;

		// `name` is mandatory.
		if (!data[k_name].isString())
			MS_THROW_ERROR("missing `name`");

		codec.name = data[k_name].asString();
		if (codec.name.empty())
			MS_THROW_ERROR("empty `name`");

		// `kind` is optional.
		if (data[k_kind].isString())
		{
			std::string kind = data[k_kind].asString();

			if (kind == "audio")
				codec.kind = Kind::AUDIO;
			else if (kind == "video")
				codec.kind = Kind::VIDEO;
			else if (kind.empty())
				codec.kind = Kind::BOTH;
			else
				MS_THROW_ERROR("invalid `kind`");
		}

		// `payloadType` is mandatory.
		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing `payloadType`");

		codec.payloadType = (uint8_t)data[k_payloadType].asUInt();

		// `clockRate` is optional.
		if (data[k_clockRate].isUInt())
			codec.clockRate = (uint32_t)data[k_clockRate].asUInt();

		// Append to the codecs vector.
		this->codecs.push_back(codec);
	}

	void RtpParameters::AddEncoding(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");

		if (!data.isObject())
			MS_THROW_ERROR("data is not an object");

		struct EncodingParameters encoding;

		// `ssrc` is mandatory.
		if (!data[k_ssrc].isUInt())
			MS_THROW_ERROR("missing `ssrc`");

		encoding.ssrc = (uint32_t)data[k_ssrc].asUInt();

		// Append to the encodings vector.
		this->encodings.push_back(encoding);
	}
}
