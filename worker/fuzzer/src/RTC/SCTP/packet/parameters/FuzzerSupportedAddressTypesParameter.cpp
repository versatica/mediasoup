#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSupportedAddressTypesParameter.hpp"

void FuzzerRtcSctpSupportedAddressTypesParameter::Fuzz(
  const RTC::SCTP::SupportedAddressTypesParameter* parameter)
{
	parameter->GetNumberOfAddressTypes();

	for (uint16_t idx{ 0 }; idx < parameter->GetNumberOfAddressTypes(); ++idx)
	{
		parameter->GetAddressTypeAt(idx);
	}
}
