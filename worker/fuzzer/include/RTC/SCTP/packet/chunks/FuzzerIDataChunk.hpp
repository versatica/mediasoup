#ifndef MS_FUZZER_RTC_SCTP_I_DATA_CHUNK_HPP
#define MS_FUZZER_RTC_SCTP_I_DATA_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"

namespace FuzzerRtcSctpIDataChunk
{
	void Fuzz(const RTC::SCTP::IDataChunk* chunk);
} // namespace FuzzerRtcSctpIDataChunk

#endif
