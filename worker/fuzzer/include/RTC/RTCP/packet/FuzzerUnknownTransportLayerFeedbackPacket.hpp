#ifndef MS_FUZZER_RTC_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"

namespace FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket
{
	void Fuzz(const RTC::RTCP::UnknownTransportLayerFeedbackPacket* packet);
} // namespace FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket

#endif
