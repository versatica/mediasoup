#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerRestartOfAnAssociationWithNewAddressesErrorCause.hpp"

void FuzzerRtcSctpRestartOfAnAssociationWithNewAddressesErrorCause::Fuzz(
  const RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause* errorCause)
{
	errorCause->HasNewAddressTlvs();
	errorCause->GetNewAddressTlvs();
	errorCause->GetNewAddressTlvsLength();
}
