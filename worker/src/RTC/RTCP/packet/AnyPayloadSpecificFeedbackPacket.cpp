#define MS_CLASS "RTC::RTCP::AnyPayloadSpecificFeedbackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<AnyPayloadSpecificFeedbackPacket::MessageType, std::string> AnyPayloadSpecificFeedbackPacket::MessageType2String =
		{
			{ AnyPayloadSpecificFeedbackPacket::MessageType::PLI, "PLI" },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::FIR, "FIR" },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::AFB, "AFB" },
		};
		// clang-format on

		/* Class methods. */

		const std::string& AnyPayloadSpecificFeedbackPacket::MessageTypeToString(MessageType messageType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			const auto it = AnyPayloadSpecificFeedbackPacket::MessageType2String.find(messageType);

			if (it == AnyPayloadSpecificFeedbackPacket::MessageType2String.end())
			{
				return Unknown;
			}

			const std::string& messageTypeString = it->second;

			return messageTypeString;
		}

		/* Instance methods. */

		AnyPayloadSpecificFeedbackPacket::AnyPayloadSpecificFeedbackPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();
		}
	} // namespace RTCP
} // namespace RTC
