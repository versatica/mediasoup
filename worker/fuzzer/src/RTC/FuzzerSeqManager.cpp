#include "fuzzer/include/RTC/FuzzerSeqManager.hpp"
#include "RTC/SeqManager.hpp"
#include "Utils.hpp"

void FuzzerRtcSeqManager::Fuzz(const uint8_t* data, size_t len)
{
	if (len < 10)
	{
		return;
	}

	RTC::SeqManager<uint16_t> seqManager;
	uint16_t output{ 0 };

	for (size_t count{ 0 }; count < 7; ++count)
	{
		static_cast<void>(seqManager.Input(Utils::Byte::Get2Bytes(data, count), output));
		seqManager.Drop(Utils::Byte::Get2Bytes(data, count + 2));
	}
}
