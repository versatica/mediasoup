#include "RTC/FuzzerRateCalculator.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/RtpPacket.hpp" // RTC::MtuSize

::RTC::RateCalculator rateCalculator;

void Fuzzer::RTC::RateCalculator::Fuzz(const uint8_t* data, size_t len)
{
	// We need at least 2 bytes of |data|.
	if (len < 2)
	{
		return;
	}

	auto size =
	  static_cast<size_t>(Utils::Crypto::GetRandomUInt(0u, static_cast<uint32_t>(::RTC::MtuSize)));
	uint64_t nowMs = DepLibUV::GetTimeMs();

	rateCalculator.Update(size, nowMs);

	// Only get rate from time to time.
	if (Utils::Byte::Get2Bytes(data, 0) % 100 == 0)
	{
		rateCalculator.GetRate(nowMs);
	}
}
