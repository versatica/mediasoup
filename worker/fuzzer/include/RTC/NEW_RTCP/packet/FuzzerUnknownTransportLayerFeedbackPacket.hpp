#ifndef MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_TRANSPORT_LAYER_FEEDBACK_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"

namespace FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket
{
	void Fuzz(const RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket* packet);
} // namespace FuzzerRtcRtcpUnknownTransportLayerFeedbackPacket

#endif
