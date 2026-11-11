#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerCookiePreservativeParameter.hpp"

void FuzzerRtcSctpCookiePreservativeParameter::Fuzz(const RTC::SCTP::CookiePreservativeParameter* parameter)
{
	parameter->GetLifeSpanIncrement();
}
