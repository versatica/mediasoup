#include "RTC/FuzzerTrendCalculator.hpp"
#include "DepLibUV.hpp"
#include "RTC/TrendCalculator.hpp"
#include "Utils.hpp"

void FuzzerRtcTrendCalculator::Fuzz(const uint8_t* data, size_t len)
{
	RTC::TrendCalculator trend;
	auto nowMs = DepLibUV::GetTimeMs();
	size_t offset{ 0u };

	while (len >= 4u)
	{
		const auto value = Utils::Byte::Get4Bytes(data, offset);

		trend.Update(value, nowMs);
		trend.GetValue();
		trend.Update(value, nowMs - 1000u);
		trend.GetValue();

		len -= 4u;
		offset += 4;
		nowMs += 500u;
	}
}
