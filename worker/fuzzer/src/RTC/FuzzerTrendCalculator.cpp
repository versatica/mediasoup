#include "RTC/FuzzerTrendCalculator.hpp"
#include "DepLibUV.hpp"
#include "RTC/TrendCalculator.hpp"
#include "Utils.hpp"

void FuzzerRtcTrendCalculator::Fuzz(const uint8_t* data, size_t len)
{
	RTC::TrendCalculator trend;
	int64_t nowMs = DepLibUV::GetTimeMsInt64();
	size_t offset{ 0 };

	while (len >= 4)
	{
		const auto value = Utils::Byte::Get4Bytes(data, offset);

		trend.Update(value, nowMs);
		trend.GetValue();
		trend.Update(value, nowMs - 1000);
		trend.GetValue();

		len -= 4;
		offset += 4;
		nowMs += 500;
	}
}
