#ifndef MS_FUZZER_RTC_SCTP_RECONFIGURATION_RESPONSE_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_RECONFIGURATION_RESPONSE_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"

namespace FuzzerRtcSctpReconfigurationResponseParameter
{
	void Fuzz(const RTC::SCTP::ReconfigurationResponseParameter* parameter);
} // namespace FuzzerRtcSctpReconfigurationResponseParameter

#endif
