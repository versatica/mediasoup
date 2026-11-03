#ifndef MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"

namespace FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket
{
	void Fuzz(const RTC::NEW_RTCP::UnknownPayloadSpecificFeedbackPacket* packet);
} // namespace FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket

#endif
