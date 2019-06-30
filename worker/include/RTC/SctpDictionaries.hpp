#ifndef MS_RTC_SCTP_DICTIONARIES_HPP
#define MS_RTC_SCTP_DICTIONARIES_HPP

#include "common.hpp"
#include "json.hpp"
#include <string>

using json = nlohmann::json;

namespace RTC
{
	class DataChannel
	{
	public:
		enum class PayloadProtocolIdentifier : uint8_t
		{
			STRING       = 51,
			BINARY       = 53,
			STRING_EMPTY = 56,
			BINARY_EMPTY = 57
		};
	};

	class SctpStreamParameters
	{
	public:
		SctpStreamParameters(){};
		explicit SctpStreamParameters(json& data);

		void FillJson(json& jsonObject) const;

	public:
		uint16_t streamId{ 0 };
		bool ordered{ true };
		uint16_t maxPacketLifeTime{ 0 };
		uint16_t maxRetransmits{ 0 };
	};
} // namespace RTC

#endif
