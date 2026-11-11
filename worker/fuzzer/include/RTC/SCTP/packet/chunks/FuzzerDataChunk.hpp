#ifndef MS_FUZZER_RTC_SCTP_DATA_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_DATA_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"

namespace FuzzerRtcSctpDataChunk
{
	void Fuzz(const RTC::SCTP::DataChunk* chunk);
} // namespace FuzzerRtcSctpDataChunk

#endif
