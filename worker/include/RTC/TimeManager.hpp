#ifndef RTC_TIME_MANAGER_HPP
#define RTC_TIME_MANAGER_HPP

#include "common.hpp"
#include <limits> // std::numeric_limits

namespace RTC
{
	class TimeManager
	{
	public:
		static constexpr uint64_t MaxValue = std::numeric_limits<uint64_t>::max();

	public:
		struct TimeLowerThan
		{
			bool operator()(uint64_t lhs, uint64_t rhs) const;
		};

		struct TimeLowerOrEqualThan
		{
			bool operator()(uint64_t lhs, uint64_t rhs) const;
		};

		struct TimeHigherThan
		{
			bool operator()(uint64_t lhs, uint64_t rhs) const;
		};

		struct TimeHigherOrEqualThan
		{
			bool operator()(uint64_t lhs, uint64_t rhs) const;
		};

	public:
		static bool IsTimeLowerThan(uint64_t lhs, uint64_t rhs);
		static bool IsTimeHigherThan(uint64_t lhs, uint64_t rhs);
		static bool IsTimeLowerOrEqualThan(uint64_t lhs, uint64_t rhs);
		static bool IsTimeHigherOrEqualThan(uint64_t lhs, uint64_t rhs);

	private:
		static const TimeLowerThan isTimeLowerThan; // NOLINT(readability-identifier-naming)
		static const TimeLowerOrEqualThan isTimeLowerOrEqualThan; // NOLINT(readability-identifier-naming)
		static const TimeHigherThan isTimeHigherThan; // NOLINT(readability-identifier-naming)
		static const TimeHigherOrEqualThan isTimeHigherOrEqualThan; // NOLINT(readability-identifier-naming)

	public:
		TimeManager() = default;
	};
} // namespace RTC

#endif
