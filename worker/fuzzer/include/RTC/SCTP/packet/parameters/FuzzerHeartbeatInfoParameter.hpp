#ifndef MS_FUZZER_RTC_SCTP_HEARTBEAT_INFO_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_HEARTBEAT_INFO_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"

namespace FuzzerRtcSctpHeartbeatInfoParameter
{
	void Fuzz(const RTC::SCTP::HeartbeatInfoParameter* parameter);
} // namespace FuzzerRtcSctpHeartbeatInfoParameter

#endif
