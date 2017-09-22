#ifndef RTC_SEQ_MANAGER_HPP
#define RTC_SEQ_MANAGER_HPP

#include "common.hpp"
#include <limits> // std::numeric_limits.
#include <set>

namespace RTC
{
	template<typename T>
	class SeqManager
	{
	public:
		static constexpr T MaxValue = std::numeric_limits<T>::max();

	private:
		struct SeqLowerThan
		{
			bool operator()(const T& lhs, const T& rhs) const;
		};

		struct SeqHigherThan
		{
			bool operator()(const T& lhs, const T& rhs) const;
		};

	private:
		static const SeqLowerThan isSeqLowerThan;
		static const SeqHigherThan isSeqHigherThan;

	public:
		static bool IsSeqLowerThan(const T& lhs, const T& rhs);
		static bool IsSeqHigherThan(const T& lhs, const T& rhs);

	public:
		SeqManager();

	public:
		void Sync(T input);
		void Drop(T input);
		bool Input(const T input, T& output);

	private:
		T base{ 0 };
		T maxOutput{ 0 };
		T maxInput{ 0 };
		std::set<T, SeqLowerThan> dropped;
	};

	// Tell the compiler not to bother instantiating this template. It is done in the .cpp file.
	extern template class SeqManager<uint16_t>;
	extern template class SeqManager<uint32_t>;
} // namespace RTC

#endif
