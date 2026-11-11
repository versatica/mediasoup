#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnresolvableAddressErrorCause.hpp"

void FuzzerRtcSctpUnresolvableAddressErrorCause::Fuzz(
  const RTC::SCTP::UnresolvableAddressErrorCause* errorCause)
{
	errorCause->HasUnresolvableAddress();
	errorCause->GetUnresolvableAddress();
	errorCause->GetUnresolvableAddressLength();
}
