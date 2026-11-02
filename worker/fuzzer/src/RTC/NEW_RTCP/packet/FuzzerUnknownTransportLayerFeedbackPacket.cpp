#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerUnknownTransportLayerFeedbackPacket.hpp"

void FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket::Fuzz(
  const RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
