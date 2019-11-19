#include "RTC/FuzzerTrendCalculator.hpp"
#include "DepLibUV.hpp"
#include "Utils.hpp"
#include "RTC/TrendCalculator.hpp"

void Fuzzer::RTC::TrendCalculator::Fuzz(const uint8_t* data, size_t len)
{
	::RTC::TrendCalculator trend;
	auto now = DepLibUV::GetTimeMs();
	size_t offset{ 0u };

	while (len >= 4u)
	{
		auto value = Utils::Byte::Get4Bytes(data, offset);

		trend.Update(value, now);
		trend.GetValue();
		trend.Update(value, now - 1000u);
		trend.GetValue();

		len -= 4u;
		offset += 4;
		now += 500u;
	}
}
