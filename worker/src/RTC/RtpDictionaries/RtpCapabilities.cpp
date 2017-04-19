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

		static const Json::StaticString JsonStringCodecs{ "codecs" };
		static const Json::StaticString JsonStringHeaderExtensions{ "headerExtensions" };
		static const Json::StaticString JsonStringFecMechanisms{ "fecMechanisms" };

		// `codecs` is optional.
		if (data[JsonStringCodecs].isArray())
		{
			auto& jsonCodecs = data[JsonStringCodecs];

			for (auto& jsonCodec : jsonCodecs)
			{
				RtpCodecParameters codec(jsonCodec, scope);

				// Append to the codecs vector.
				this->codecs.push_back(codec);
			}
		}

		// `headerExtensions` is optional.
		if (data[JsonStringHeaderExtensions].isArray())
		{
			auto& jsonArray = data[JsonStringHeaderExtensions];

			for (auto& i : jsonArray)
			{
				RtpHeaderExtension headerExtension(i);

				// If a known header extension, append to the headerExtensions vector.
				if (headerExtension.type != RtpHeaderExtensionUri::Type::UNKNOWN)
					this->headerExtensions.push_back(headerExtension);
			}
		}

		// `fecMechanisms` is optional.
		if (data[JsonStringFecMechanisms].isArray())
		{
			auto& jsonArray = data[JsonStringFecMechanisms];

			for (const auto& i : jsonArray)
			{
				if (!i.isString())
					MS_THROW_ERROR("invalid RtpCapabilities.fecMechanisms");

				// Append to the fecMechanisms vector.
				this->fecMechanisms.push_back(i.asString());
			}
		}

		// Validate RTP capabilities.

		ValidateCodecs(scope);
	}

	Json::Value RtpCapabilities::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringCodecs{ "codecs" };
		static const Json::StaticString JsonStringHeaderExtensions{ "headerExtensions" };
		static const Json::StaticString JsonStringFecMechanisms{ "fecMechanisms" };

		Json::Value json(Json::objectValue);

		// Add `codecs`.
		json[JsonStringCodecs] = Json::arrayValue;

		for (auto& entry : this->codecs)
		{
			json[JsonStringCodecs].append(entry.ToJson());
		}

		// Add `headerExtensions`.
		json[JsonStringHeaderExtensions] = Json::arrayValue;

		for (auto& entry : this->headerExtensions)
		{
			json[JsonStringHeaderExtensions].append(entry.ToJson());
		}

		// Add `fecMechanisms`.
		json[JsonStringFecMechanisms] = Json::arrayValue;

		for (auto& entry : this->fecMechanisms)
		{
			json[JsonStringFecMechanisms].append(entry);
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
} // namespace RTC
