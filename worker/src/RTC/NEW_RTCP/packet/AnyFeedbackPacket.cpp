#define MS_CLASS "RTC::NEW_RTCP::AnyFeedbackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/AnyFeedbackPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Instance methods. */

		AnyFeedbackPacket::AnyFeedbackPacket(uint8_t* buffer, size_t bufferLength)
		  : Packet(buffer, bufferLength)
		{
			MS_TRACE();
		}

		void AnyFeedbackPacket::SetSenderSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, ssrc);
		}

		void AnyFeedbackPacket::SetMediaSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, ssrc);
		}

		void AnyFeedbackPacket::DumpFeedbackCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "  sender ssrc: %" PRIu32, GetSenderSsrc());
			MS_DUMP_CLEAN(indentation, "  media ssrc: %" PRIu32, GetMediaSsrc());
		}
	} // namespace NEW_RTCP
} // namespace RTC
