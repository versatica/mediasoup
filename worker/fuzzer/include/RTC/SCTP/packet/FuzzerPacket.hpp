#ifndef MS_FUZZER_RTC_SCTP_PACKET_HPP
#define MS_FUZZER_RTC_SCTP_PACKET_HPP

#include "common.hpp"

namespace FuzzerRtcSctpPacket
{
	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcSctpPacket

#endif
