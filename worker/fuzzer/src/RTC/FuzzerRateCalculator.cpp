#include "RTC/FuzzerRateCalculator.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RateCalculator.hpp"

namespace
{
	// NOLINTBEGIN(readability-identifier-naming)
	RTC::RateCalculator rateCalculator;
	uint64_t nowMs;
	// NOLINTEND(readability-identifier-naming)

	int init()
	{
		nowMs = DepLibUV::GetTimeMs();

		return 0;
	}
} // namespace

void FuzzerRtcRateCalculator::Fuzz(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	// NOLINTNEXTLINE(readability-identifier-naming)
	thread_local const int unused = init();

	// Avoid [-Wunused-variable].
	(void)unused;

	// We need at least 2 bytes of |data|.
	if (len < 2)
	{
		return;
	}

	auto size = Utils::Crypto::GetRandomUInt<size_t>(0u, static_cast<uint32_t>(RTC::Consts::MtuSize));

	nowMs += Utils::Crypto::GetRandomUInt<uint64_t>(0u, 1000u);

	rateCalculator.Update(size, nowMs);

	// Only get rate from time to time.
	if (Utils::Byte::Get2Bytes(data, 0) % 100 == 0)
	{
		rateCalculator.GetRate(nowMs);
	}
}
