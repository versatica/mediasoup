#ifndef MS_FUZZER_RTC_SCTP_COOKIE_ACK_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_COOKIE_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"

namespace FuzzerRtcSctpCookieAckChunk
{
	void Fuzz(const RTC::SCTP::CookieAckChunk* chunk);
} // namespace FuzzerRtcSctpCookieAckChunk

#endif
