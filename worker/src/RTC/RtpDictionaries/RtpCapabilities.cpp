#define MS_CLASS "RTC::RtpCapabilities"

#include "RTC/RtpDictionaries.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpCapabilities::RtpCapabilities(Json::Value& data)
	{
		MS_TRACE();

		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_fecMechanisms("fecMechanisms");

		// `codecs` is mandatory.
		if (data[k_codecs].isArray())
		{
			auto& json_codecs = data[k_codecs];

			for (Json::UInt i = 0; i < json_codecs.size(); i++)
			{
				RtpCodecCapability codec(json_codecs[i]);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}
		else
		{
			MS_THROW_ERROR("missing RtpCapabilities.codecs");
		}

		// `headerExtensions` is optional.
		if (data[k_headerExtensions].isArray())
		{
			auto& json_array = data[k_headerExtensions];

			for (Json::UInt i = 0; i < json_array.size(); i++)
			{
				RtpHeaderExtension headerExtension(json_array[i]);

				// Append to the headerExtensions vector.
				this->headerExtensions.push_back(headerExtension);
			}
		}

		// `fecMechanisms` is optional.
		if (data[k_fecMechanisms].isArray())
		{
			auto& json_array = data[k_fecMechanisms];

			for (Json::UInt i = 0; i < json_array.size(); i++)
			{
				if (!json_array[i].isString())
					MS_THROW_ERROR("invalid RtpCapabilities.fecMechanisms");

				// Append to the fecMechanisms vector.
				this->fecMechanisms.push_back(json_array[i].asString());
			}
		}

		// Validate RTP capabilities.

		ValidateCodecs();
	}

	RtpCapabilities::~RtpCapabilities()
	{
		MS_TRACE();
	}

	Json::Value RtpCapabilities::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_fecMechanisms("fecMechanisms");

		Json::Value json(Json::objectValue);

		// Add `codecs`.
		if (!this->codecs.empty())
		{
			json[k_codecs] = Json::arrayValue;

			for (auto& entry : this->codecs)
			{
				json[k_codecs].append(entry.toJson());
			}
		}

		// Add `headerExtensions`.
		if (!this->headerExtensions.empty())
		{
			json[k_headerExtensions] = Json::arrayValue;

			for (auto& entry : this->headerExtensions)
			{
				json[k_headerExtensions].append(entry.toJson());
			}
		}

		// Add `fecMechanisms`.
		if (!this->fecMechanisms.empty())
		{
			json[k_fecMechanisms] = Json::arrayValue;

			for (auto& entry : this->fecMechanisms)
			{
				json[k_fecMechanisms].append(entry);
			}
		}

		return json;
	}

	void RtpCapabilities::ValidateCodecs()
	{
		MS_TRACE();

		// Must be at least one codec.
		if (this->codecs.size() == 0)
			MS_THROW_ERROR("empty RtpCapabilities.codecs");

		// Preferred payload types must be unique.

		std::unordered_set<uint8_t> preferredPayloadTypes;

		for (auto& codec : this->codecs)
		{
			// Preferred payload types must be unique.
			if (preferredPayloadTypes.find(codec.preferredPayloadType) != preferredPayloadTypes.end())
				MS_THROW_ERROR("duplicated codec.preferredPayloadType");
			else
				preferredPayloadTypes.insert(codec.preferredPayloadType);
		}
	}
}
