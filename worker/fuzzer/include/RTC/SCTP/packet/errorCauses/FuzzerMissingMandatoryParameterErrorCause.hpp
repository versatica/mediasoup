#ifndef MS_FUZZER_RTC_SCTP_MISSING_MANDATORY_PARAMETER_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_MISSING_MANDATORY_PARAMETER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/MissingMandatoryParameterErrorCause.hpp"

namespace FuzzerRtcSctpMissingMandatoryParameterErrorCause
{
	void Fuzz(const RTC::SCTP::MissingMandatoryParameterErrorCause* errorCause);
} // namespace FuzzerRtcSctpMissingMandatoryParameterErrorCause

#endif
