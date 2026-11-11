#ifndef MS_FUZZER_RTC_SCTP_OUT_OF_RESOURCE_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_OUT_OF_RESOURCE_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"

namespace FuzzerRtcSctpOutOfResourceErrorCause
{
	void Fuzz(const RTC::SCTP::OutOfResourceErrorCause* errorCause);
} // namespace FuzzerRtcSctpOutOfResourceErrorCause

#endif
