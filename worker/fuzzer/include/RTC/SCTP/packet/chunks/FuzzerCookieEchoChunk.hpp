#ifndef MS_FUZZER_RTC_SCTP_COOKIE_ECHO_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_COOKIE_ECHO_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"

namespace FuzzerRtcSctpCookieEchoChunk
{
	void Fuzz(const RTC::SCTP::CookieEchoChunk* chunk);
} // namespace FuzzerRtcSctpCookieEchoChunk

#endif
