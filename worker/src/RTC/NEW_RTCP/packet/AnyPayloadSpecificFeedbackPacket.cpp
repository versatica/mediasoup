#define MS_CLASS "RTC::NEW_RTCP::AnyPayloadSpecificFeedbackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<AnyPayloadSpecificFeedbackPacket::MessageType, std::string> AnyPayloadSpecificFeedbackPacket::MessageType2String =
		{
			{ AnyPayloadSpecificFeedbackPacket::MessageType::PLI,   "PLI"   },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::SLI,   "SLI"   },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::RPSI,  "RPSI"  },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::FIR,   "FIR"   },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::TSTR,  "TSTR"  },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::TSTN,  "TSTN"  },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::VBCM,  "VBCM"  },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::PSLEI, "PSLEI" },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::ROI,   "ROI"   },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::AFB,   "AFB"   },
			{ AnyPayloadSpecificFeedbackPacket::MessageType::EXT,   "EXT"   },
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
	} // namespace NEW_RTCP
} // namespace RTC
