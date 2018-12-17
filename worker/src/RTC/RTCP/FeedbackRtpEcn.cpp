#define MS_CLASS "RTC::RTCP::FeedbackRtpEcn"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		size_t FeedbackRtpEcnItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add minimum header.
			std::memcpy(buffer, this->header, sizeof(Header));

			return sizeof(Header);
		}

		void FeedbackRtpEcnItem::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<FeedbackRtpEcnItem>");
			MS_DEBUG_DEV("  sequence number    : %" PRIu32, this->GetSequenceNumber());
			MS_DEBUG_DEV("  ect0 counter       : %" PRIu32, this->GetEct0Counter());
			MS_DEBUG_DEV("  ect1 counter       : %" PRIu32, this->GetEct1Counter());
			MS_DEBUG_DEV("  ecn ce counter     : %" PRIu16, this->GetEcnCeCounter());
			MS_DEBUG_DEV("  not ect counter    : %" PRIu16, this->GetNotEctCounter());
			MS_DEBUG_DEV("  lost packets       : %" PRIu16, this->GetLostPackets());
			MS_DEBUG_DEV("  duplicated packets : %" PRIu16, this->GetDuplicatedPackets());
			MS_DEBUG_DEV("</FeedbackRtpEcnItem>");
		}
	} // namespace RTCP
} // namespace RTC
