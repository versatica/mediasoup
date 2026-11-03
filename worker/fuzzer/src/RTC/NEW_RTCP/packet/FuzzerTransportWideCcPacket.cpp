#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerTransportWideCcPacket.hpp"

void FuzzerRtcRtcpTransportWideCcPacket::Fuzz(const RTC::NEW_RTCP::TransportWideCcPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetBaseSequenceNumber();
	packet->GetPacketStatusCount();
	packet->GetBaseTimeUs();
	packet->GetBaseDeltaUs(0);
	packet->GetFeedbackPacketCount();
	packet->HasDeltas();
	packet->GetReceivedRtpPackets();
	packet->ForAllRtpPackets(
	  [](uint16_t /*sequenceNumber*/, std::optional<int64_t> /*deltaSinceBaseUs*/) -> void
	  {
	  });
}
