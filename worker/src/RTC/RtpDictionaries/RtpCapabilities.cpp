#define MS_CLASS "RTC::RtpCapabilities"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpCapabilities::RtpCapabilities(Json::Value& data, RTC::Scope scope)
	{
		MS_TRACE();

		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_fecMechanisms("fecMechanisms");

		// `codecs` is optional.
		if (data[k_codecs].isArray())
		{
			auto& jsonCodecs = data[k_codecs];

			for (Json::UInt i = 0; i < jsonCodecs.size(); ++i)
			{
				RtpCodecParameters codec(jsonCodecs[i], scope);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}

		// `headerExtensions` is optional.
		if (data[k_headerExtensions].isArray())
		{
			auto& jsonArray = data[k_headerExtensions];

			for (Json::UInt i = 0; i < jsonArray.size(); ++i)
			{
				RtpHeaderExtension headerExtension(jsonArray[i]);

				// If a known header extension, append to the headerExtensions vector.
				if (headerExtension.type != RtpHeaderExtensionUri::Type::UNKNOWN)
					this->headerExtensions.push_back(headerExtension);
			}
		}

		// `fecMechanisms` is optional.
		if (data[k_fecMechanisms].isArray())
		{
			auto& jsonArray = data[k_fecMechanisms];

			for (Json::UInt i = 0; i < jsonArray.size(); ++i)
			{
				if (!jsonArray[i].isString())
					MS_THROW_ERROR("invalid RtpCapabilities.fecMechanisms");

				// Append to the fecMechanisms vector.
				this->fecMechanisms.push_back(jsonArray[i].asString());
			}
		}

		// Validate RTP capabilities.

		ValidateCodecs(scope);
	}

	Json::Value RtpCapabilities::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_codecs("codecs");
		static const Json::StaticString k_headerExtensions("headerExtensions");
		static const Json::StaticString k_fecMechanisms("fecMechanisms");

		Json::Value json(Json::objectValue);

		// Add `codecs`.
		json[k_codecs] = Json::arrayValue;

		for (auto& entry : this->codecs)
		{
			json[k_codecs].append(entry.toJson());
		}

		// Add `headerExtensions`.
		json[k_headerExtensions] = Json::arrayValue;

		for (auto& entry : this->headerExtensions)
		{
			json[k_headerExtensions].append(entry.toJson());
		}

		// Add `fecMechanisms`.
		json[k_fecMechanisms] = Json::arrayValue;

		for (auto& entry : this->fecMechanisms)
		{
			json[k_fecMechanisms].append(entry);
		}

		return json;
	}

	void RtpCapabilities::ReduceHeaderExtensions(
	    std::vector<RTC::RtpHeaderExtension>& supportedHeaderExtensions)
	{
		MS_TRACE();

		std::vector<RTC::RtpHeaderExtension> updatedHeaderExtensions;

		for (auto& headerExtension : this->headerExtensions)
		{
			for (auto& supportedHeaderExtension : supportedHeaderExtensions)
			{
				if (headerExtension.type == supportedHeaderExtension.type &&
				    (headerExtension.kind == supportedHeaderExtension.kind ||
				     supportedHeaderExtension.kind == RTC::Media::Kind::ALL))
				{
					// Set the same id and other properties.
					headerExtension.preferredId      = supportedHeaderExtension.preferredId;
					headerExtension.preferredEncrypt = supportedHeaderExtension.preferredEncrypt;

					updatedHeaderExtensions.push_back(headerExtension);

					break;
				}
			}
		}

		this->headerExtensions = updatedHeaderExtensions;
	}

	void RtpCapabilities::ReduceFecMechanisms(std::vector<std::string>& supportedFecMechanisms)
	{
		MS_TRACE();

		std::vector<std::string> updatedFecMechanisms;

		for (auto& fecMechanism : this->fecMechanisms)
		{
			for (auto& supportedFecMechanism : supportedFecMechanisms)
			{
				if (fecMechanism == supportedFecMechanism)
				{
					updatedFecMechanisms.push_back(fecMechanism);

					break;
				}
			}
		}

		this->fecMechanisms = updatedFecMechanisms;
	}

	inline void RtpCapabilities::ValidateCodecs(RTC::Scope scope)
	{
		MS_TRACE();

		if (scope == RTC::Scope::PEER_CAPABILITY)
		{
			// Payload types must be unique.

			std::unordered_set<uint8_t> payloadTypes;

			for (auto& codec : this->codecs)
			{
				if (payloadTypes.find(codec.payloadType) != payloadTypes.end())
					MS_THROW_ERROR("duplicated codec.payloadType");
				else
					payloadTypes.insert(codec.payloadType);
			}
		}
	}
}
