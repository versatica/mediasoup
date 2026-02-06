#ifndef MS_FUZZER_RTC_ICE_STUN_PACKET_HPP
#define MS_FUZZER_RTC_ICE_STUN_PACKET_HPP

#include "common.hpp"

namespace FuzzerRtcIceStunPacket
{
	void Fuzz(const uint8_t* data, size_t len);
}

#endif
