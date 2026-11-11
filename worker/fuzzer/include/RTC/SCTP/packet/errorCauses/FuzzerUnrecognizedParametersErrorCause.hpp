#ifndef MS_FUZZER_RTC_SCTP_UNRECOGNIZED_PARAMETERS_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_UNRECOGNIZED_PARAMETERS_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"

namespace FuzzerRtcSctpUnrecognizedParametersErrorCause
{
	void Fuzz(const RTC::SCTP::UnrecognizedParametersErrorCause* errorCause);
} // namespace FuzzerRtcSctpUnrecognizedParametersErrorCause

#endif
