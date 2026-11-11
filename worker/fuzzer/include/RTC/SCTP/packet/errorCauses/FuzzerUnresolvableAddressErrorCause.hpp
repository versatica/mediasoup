#ifndef MS_FUZZER_RTC_SCTP_UNRESOLVABLE_ADDRESS_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_UNRESOLVABLE_ADDRESS_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/UnresolvableAddressErrorCause.hpp"

namespace FuzzerRtcSctpUnresolvableAddressErrorCause
{
	void Fuzz(const RTC::SCTP::UnresolvableAddressErrorCause* errorCause);
} // namespace FuzzerRtcSctpUnresolvableAddressErrorCause

#endif
