#ifndef MS_FUZZER_RTC_RTCP_FIR_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_FIR_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/FirPacket.hpp"

namespace FuzzerRtcRtcpFirPacket
{
	void Fuzz(const RTC::RTCP::FirPacket* packet);
} // namespace FuzzerRtcRtcpFirPacket

#endif
