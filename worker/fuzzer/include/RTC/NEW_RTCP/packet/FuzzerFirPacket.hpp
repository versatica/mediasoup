#ifndef MS_FUZZER_RTC_NEW_RTCP_FIR_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_FIR_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/FirPacket.hpp"

namespace FuzzerRtcRtcpFirPacket
{
	void Fuzz(const RTC::NEW_RTCP::FirPacket* packet);
} // namespace FuzzerRtcRtcpFirPacket

#endif
