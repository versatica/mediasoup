#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerStateCookieParameter.hpp"

void FuzzerRtcSctpStateCookieParameter::Fuzz(const RTC::SCTP::StateCookieParameter* parameter)
{
	parameter->HasCookie();
	parameter->GetCookie();
	parameter->GetCookieLength();
}
