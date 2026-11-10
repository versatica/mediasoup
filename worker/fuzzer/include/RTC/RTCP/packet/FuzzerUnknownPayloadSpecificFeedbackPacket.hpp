#ifndef MS_FUZZER_RTC_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_UNKNOWN_PAYLOAD_SPECIFIC_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/UnknownPayloadSpecificFeedbackPacket.hpp"

namespace FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket
{
	void Fuzz(const RTC::RTCP::UnknownPayloadSpecificFeedbackPacket* packet);
} // namespace FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket

#endif
