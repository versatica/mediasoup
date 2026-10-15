#ifndef RTC_SEQ_MANAGER_HPP
#define RTC_SEQ_MANAGER_HPP

#include "common.hpp"
#include <limits> // std::numeric_limits
#include <vector>

namespace RTC
{
	// T is the base type (uint16_t, uint32_t, ...).
	// N is the max number of bits used in T.
	template<typename T, uint8_t N = 0>
	class SeqManager
	{
	public:
		static constexpr T MaxValue = (N == 0) ? std::numeric_limits<T>::max() : ((1 << N) - 1);

	public:
		struct SeqLowerThan
		{
			bool operator()(T lhs, T rhs) const;
		};

		struct SeqHigherThan
		{
			bool operator()(T lhs, T rhs) const;
		};

	public:
		static bool IsSeqHigherThan(T lhs, T rhs);
		static bool IsSeqLowerThan(T lhs, T rhs);

	public:
		SeqManager() = default;
		explicit SeqManager(T initialOutput);

	public:
		void Sync(T input);
		void Drop(T input);
		bool Input(T input, T& output);
		T GetMaxInput() const;
		T GetMaxOutput() const;

	private:
		void AddDropped(T input);
		void ClearDropped();
		// Number of dropped inputs held, excluding the already consumed prefix.
		size_t DroppedSize() const;

	private:
		// Whether at least a sequence number has been inserted.
		bool started{ false };
		T initialOutput{ 0 };
		T base{ 0 };
		T maxOutput{ 0 };
		T maxInput{ 0 };
		T maxDropped{ 0 };
		T maxForwarded{ 0 };
		// Dropped inputs, sorted with 'SeqLowerThan'.
		std::vector<T> dropped;
		// Entries before this index are only waiting to be physically erased.
		// See 'ClearDropped()'.
		size_t droppedStart{ 0 };
	};
} // namespace RTC

#endif
