#ifndef MS_FUZZER_RTC_SCTP_ADD_OUTGOING_STREAMS_REQUEST_PARAMETER_HPP
#define MS_FUZZER_RTC_SCTP_ADD_OUTGOING_STREAMS_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/parameters/AddOutgoingStreamsRequestParameter.hpp"

namespace FuzzerRtcSctpAddOutgoingStreamsRequestParameter
{
	void Fuzz(const RTC::SCTP::AddOutgoingStreamsRequestParameter* parameter);
} // namespace FuzzerRtcSctpAddOutgoingStreamsRequestParameter

#endif
