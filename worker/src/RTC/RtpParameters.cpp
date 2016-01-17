#define MS_CLASS "RTC::RtpParameters"

#include "RTC/RtpParameters.h"
// #include "Utils.h"  // TODO: let's see
#include "MediaSoupError.h"
#include "Logger.h"
#include <memory>  // std::addressof()

namespace RTC
{
	/* Instance methods. */

	RtpParameters::RtpParameters(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_muxId("muxId");
		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_encodings("encodings");

		if (data[k_muxId].isString())
		{
			this->muxId = data[k_muxId].asString();
		}

		if (data[k_codecs].isArray())
		{
			auto json_codecs = data[k_codecs];

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
				AddCodec(json_codecs[i]);
		}

		if (data[k_encodings].isArray())
		{
			auto json_encodings = data[k_encodings];

			for (Json::UInt i = 0; i < json_encodings.size(); i++)
				AddEncoding(json_encodings[i]);
		}
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
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_encodings("encodings");
		static const Json::StaticString k_ssrc("ssrc");

		Json::Value json;

		// Add `muxId`.
		json[k_muxId] = this->muxId;

		// Add `codecs`.
		json[k_codecs] = Json::arrayValue;
		for (auto it = this->codecs.begin(); it != this->codecs.end(); ++it)
		{
			CodecParameters* codec = std::addressof(*it);
			Json::Value json_codec;

			json_codec[k_name] = codec->name;

			json_codec[k_payloadType] = (Json::UInt)codec->payloadType;

			if (codec->clockRate)
				json_codec[k_clockRate] = (Json::UInt)codec->clockRate;

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
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_clockRate("clockRate");

		if (!data.isObject())
			MS_THROW_ERROR("data is not an object");

		struct CodecParameters codec;

		if (!data[k_name].isString())
			MS_THROW_ERROR("missing `name`");

		codec.name = data[k_name].asString();

		if (codec.name.empty())
			MS_THROW_ERROR("empty `name`");

		if (!data[k_payloadType].isUInt())
			MS_THROW_ERROR("missing `payloadType`");

		codec.payloadType = (uint8_t)data[k_payloadType].asUInt();

		if (data[k_clockRate].isUInt())
			codec.clockRate = (uint8_t)data[k_clockRate].asUInt();

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

		if (data[k_ssrc].isUInt())
			encoding.ssrc = (uint32_t)data[k_ssrc].asUInt();

		// Append to the encodings vector.
		this->encodings.push_back(encoding);
	}
}
