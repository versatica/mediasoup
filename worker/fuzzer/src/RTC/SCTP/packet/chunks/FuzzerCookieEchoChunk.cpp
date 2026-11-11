#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerCookieEchoChunk.hpp"

void FuzzerRtcSctpCookieEchoChunk::Fuzz(const RTC::SCTP::CookieEchoChunk* chunk)
{
	chunk->HasCookie();
	chunk->GetCookie();
	chunk->GetCookieLength();
}
