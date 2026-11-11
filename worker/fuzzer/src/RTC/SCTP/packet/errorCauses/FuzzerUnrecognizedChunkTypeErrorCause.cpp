#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnrecognizedChunkTypeErrorCause.hpp"

void FuzzerRtcSctpUnrecognizedChunkTypeErrorCause::Fuzz(
  const RTC::SCTP::UnrecognizedChunkTypeErrorCause* errorCause)
{
	errorCause->HasUnrecognizedChunk();
	errorCause->GetUnrecognizedChunk();
	errorCause->GetUnrecognizedChunkLength();
}
