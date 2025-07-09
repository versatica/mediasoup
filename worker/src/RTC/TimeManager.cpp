#define MS_CLASS "RTC::TimeManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TimeManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	bool TimeManager::TimeLowerThan::operator()(uint64_t lhs, uint64_t rhs) const
	{
		// clang-format off
		return ((rhs > lhs) && (rhs - lhs <= TimeManager::MaxValue / 2)) ||
		       ((lhs > rhs) && (lhs - rhs > TimeManager::MaxValue / 2));
		// clang-format on
	}

	bool TimeManager::TimeLowerOrEqualThan::operator()(uint64_t lhs, uint64_t rhs) const
	{
		// clang-format off
		return (lhs == rhs) ||
		       ((rhs > lhs) && (rhs - lhs <= TimeManager::MaxValue / 2)) ||
		       ((lhs > rhs) && (lhs - rhs > TimeManager::MaxValue / 2));
		// clang-format on
	}

	bool TimeManager::TimeHigherThan::operator()(uint64_t lhs, uint64_t rhs) const
	{
		// clang-format off
		return ((lhs > rhs) && (lhs - rhs <= TimeManager::MaxValue / 2)) ||
		       ((rhs > lhs) && (rhs - lhs > TimeManager::MaxValue / 2));
		// clang-format on
	}

	bool TimeManager::TimeHigherOrEqualThan::operator()(uint64_t lhs, uint64_t rhs) const
	{
		// clang-format off
		return (lhs == rhs) ||
		       ((lhs > rhs) && (lhs - rhs <= TimeManager::MaxValue / 2)) ||
		       ((rhs > lhs) && (rhs - lhs > TimeManager::MaxValue / 2));
		// clang-format on
	}

	const typename TimeManager::TimeLowerThan TimeManager::isTimeLowerThan{};

	const typename TimeManager::TimeLowerOrEqualThan TimeManager::isTimeLowerOrEqualThan{};

	const typename TimeManager::TimeHigherThan TimeManager::isTimeHigherThan{};

	const typename TimeManager::TimeHigherOrEqualThan TimeManager::isTimeHigherOrEqualThan{};

	bool TimeManager::IsTimeLowerThan(uint64_t lhs, uint64_t rhs)
	{
		return TimeManager::isTimeLowerThan(lhs, rhs);
	}

	bool TimeManager::IsTimeLowerOrEqualThan(uint64_t lhs, uint64_t rhs)
	{
		return TimeManager::isTimeLowerOrEqualThan(lhs, rhs);
	}

	bool TimeManager::IsTimeHigherThan(uint64_t lhs, uint64_t rhs)
	{
		return TimeManager::isTimeHigherThan(lhs, rhs);
	}

	bool TimeManager::IsTimeHigherOrEqualThan(uint64_t lhs, uint64_t rhs)
	{
		return TimeManager::isTimeHigherOrEqualThan(lhs, rhs);
	}
} // namespace RTC
