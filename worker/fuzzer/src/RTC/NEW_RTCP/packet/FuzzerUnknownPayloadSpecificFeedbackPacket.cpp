#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerUnknownPayloadSpecificFeedbackPacket.hpp"

void FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
  const RTC::NEW_RTCP::UnknownPayloadSpecificFeedbackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
