#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownTransportLayerFeedbackPacket.hpp"

void FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket::Fuzz(
  const RTC::RTCP::UnknownTransportLayerFeedbackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
