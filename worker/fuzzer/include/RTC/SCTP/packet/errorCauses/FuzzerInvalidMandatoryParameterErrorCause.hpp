#ifndef MS_FUZZER_RTC_SCTP_INVALID_MANDATORY_PARAMETER_ERROR_CAUSE_HPP
#define MS_FUZZER_RTC_SCTP_INVALID_MANDATORY_PARAMETER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidMandatoryParameterErrorCause.hpp"

namespace FuzzerRtcSctpInvalidMandatoryParameterErrorCause
{
	void Fuzz(const RTC::SCTP::InvalidMandatoryParameterErrorCause* errorCause);
} // namespace FuzzerRtcSctpInvalidMandatoryParameterErrorCause

#endif
