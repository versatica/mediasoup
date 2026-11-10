#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownPayloadSpecificFeedbackPacket.hpp"

void FuzzerRtcRtcpUnknownPayloadSpecificFeedbackPacket::Fuzz(
  const RTC::RTCP::UnknownPayloadSpecificFeedbackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
