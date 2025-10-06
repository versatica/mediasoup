#define MS_CLASS "RTC::RTCP::FeedbackRtpEcn"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "Logger.hpp"
#include <cstring> // std::memcpy

namespace RTC
{
	namespace RTCP
	{
		size_t FeedbackRtpEcnItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Add minimum header.
			std::memcpy(buffer, this->header, HeaderSize);

			return HeaderSize;
		}

		void FeedbackRtpEcnItem::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<FeedbackRtpEcnItem>");
			MS_DUMP_CLEAN(indentation, "  sequence number: %" PRIu32, this->GetSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  ect0 counter: %" PRIu32, this->GetEct0Counter());
			MS_DUMP_CLEAN(indentation, "  ect1 counter: %" PRIu32, this->GetEct1Counter());
			MS_DUMP_CLEAN(indentation, "  ecn ce counter: %" PRIu16, this->GetEcnCeCounter());
			MS_DUMP_CLEAN(indentation, "  not ect counter: %" PRIu16, this->GetNotEctCounter());
			MS_DUMP_CLEAN(indentation, "  lost packets: %" PRIu16, this->GetLostPackets());
			MS_DUMP_CLEAN(indentation, "  duplicated packets: %" PRIu16, this->GetDuplicatedPackets());
			MS_DUMP_CLEAN(indentation, "</FeedbackRtpEcnItem>");
		}
	} // namespace RTCP
} // namespace RTC
