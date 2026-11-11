#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUserInitiatedAbortErrorCause.hpp"

void FuzzerRtcSctpUserInitiatedAbortErrorCause::Fuzz(
  const RTC::SCTP::UserInitiatedAbortErrorCause* errorCause)
{
	errorCause->HasUpperLayerAbortReason();
	errorCause->GetUpperLayerAbortReason();
}
