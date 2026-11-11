#ifndef MS_FUZZER_RTC_SCTP_ADD_INCOMING_STREAMS_REQUEST_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_ADD_INCOMING_STREAMS_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/AddIncomingStreamsRequestParameter.hpp"

namespace FuzzerRtcSctpAddIncomingStreamsRequestParameter
{
	void Fuzz(const RTC::SCTP::AddIncomingStreamsRequestParameter* parameter);
} // namespace FuzzerRtcSctpAddIncomingStreamsRequestParameter

#endif
