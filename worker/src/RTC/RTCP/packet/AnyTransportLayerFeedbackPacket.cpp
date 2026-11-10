#define MS_CLASS "RTC::RTCP::AnyTransportLayerFeedbackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/AnyTransportLayerFeedbackPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<AnyTransportLayerFeedbackPacket::MessageType, std::string> AnyTransportLayerFeedbackPacket::MessageType2String =
		{
			{ AnyTransportLayerFeedbackPacket::MessageType::NACK, "NACK" },
			{ AnyTransportLayerFeedbackPacket::MessageType::TCC,  "TCC"  },
		};
		// clang-format on

		/* Class methods. */

		const std::string& AnyTransportLayerFeedbackPacket::MessageTypeToString(MessageType messageType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			const auto it = AnyTransportLayerFeedbackPacket::MessageType2String.find(messageType);

			if (it == AnyTransportLayerFeedbackPacket::MessageType2String.end())
			{
				return Unknown;
			}

			const std::string& messageTypeString = it->second;

			return messageTypeString;
		}

		/* Instance methods. */

		AnyTransportLayerFeedbackPacket::AnyTransportLayerFeedbackPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();
		}
	} // namespace RTCP
} // namespace RTC
