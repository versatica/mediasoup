#include "fuzzer/include/RTC/FuzzerRateCalculator.hpp"
#include "DepLibUV.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RateCalculator.hpp"
#include "Utils.hpp"

void FuzzerRtcRateCalculator::Fuzz(const uint8_t* data, size_t len)
{
	// Initialize our stuff just once.
	static thread_local RTC::RateCalculator rateCalculator;
	static thread_local int64_t nowMs{ DepLibUV::GetTimeMs() };

	// We need at least 2 bytes of |data|.
	if (len < 2)
	{
		return;
	}

	const auto size = Utils::Crypto::GetRandomUInt<size_t>(0, RTC::Consts::IncomingRtpPacketMaxSize);

	nowMs += static_cast<int64_t>(Utils::Crypto::GetRandomUInt<uint64_t>(0, 1000));

	rateCalculator.Update(size, nowMs);

	// Only get rate from time to time.
	if (Utils::Byte::Get2Bytes(data, 0) % 100 == 0)
	{
		rateCalculator.GetRate(nowMs);
	}
}
