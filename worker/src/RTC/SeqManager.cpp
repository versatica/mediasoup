#define MS_CLASS "RTC::SeqManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SeqManager.hpp"
#include "Logger.hpp"
#include <iterator>

namespace RTC
{
	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqLowerThan::operator()(const T lhs, const T rhs) const
	{
		return ((rhs > lhs) && (rhs - lhs <= MaxValue / 2)) ||
		       ((lhs > rhs) && (lhs - rhs > MaxValue / 2));
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqHigherThan::operator()(const T lhs, const T rhs) const
	{
		return ((lhs > rhs) && (lhs - rhs <= MaxValue / 2)) ||
		       ((rhs > lhs) && (rhs - lhs > MaxValue / 2));
	}

	template<typename T, uint8_t N>
	const typename SeqManager<T, N>::SeqLowerThan SeqManager<T, N>::isSeqLowerThan{};

	template<typename T, uint8_t N>
	const typename SeqManager<T, N>::SeqHigherThan SeqManager<T, N>::isSeqHigherThan{};

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqLowerThan(const T lhs, const T rhs)
	{
		return isSeqLowerThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqHigherThan(const T lhs, const T rhs)
	{
		return isSeqHigherThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Sync(T input)
	{
		// Update base.
		this->base = (this->maxOutput - input) & MaxValue;

		// Update maxInput.
		this->maxInput = input;

		// Clear dropped set.
		this->dropped.clear();
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Drop(T input)
	{
		// Mark as dropped if 'input' is higher than anyone already processed.
		if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
		{
			this->dropped.insert(input);
		}
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Offset(T offset)
	{
		this->base = (this->base + offset) & MaxValue;
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::Input(const T input, T& output)
	{
		auto base = this->base;

		// There are dropped inputs. Synchronize.
		if (!this->dropped.empty())
		{
			// Check whether this input was dropped.
			if (this->dropped.find(input) != this->dropped.end())
			{
				MS_DEBUG_DEV("trying to send a dropped input");

				return false;
			}

			// Dropped entries count that must be considered for the output.
			size_t count{ 0 };

			/*
			 * Consider values lower than input and those higher that input
			 * which belong to a previous cycle.
			 */
			for (const auto& value : this->dropped)
			{
				// clang-format off
				if
				(
					IsSeqLowerThan(value, input) ||
					(
					 (value > input && (value - input > MaxValue / 3)) ||
					 (value < input && (input - value > MaxValue / 3))
					)
				)
				// clang-format on
				{
					count++;
				}
			}

			base = (this->base - count) & MaxValue;
		}

		output = (input + base) & MaxValue;

		if (!this->started)
		{
			this->started = true;

			this->maxInput  = input;
			this->maxOutput = output;
		}
		else
		{
			// New input is higher than the maximum seen. But less than acceptable units higher.
			// Keep it as the maximum seen. See Drop().
			if (IsSeqHigherThan(input, this->maxInput))
			{
				this->maxInput = input;
			}

			// New output is higher than the maximum seen. But less than acceptable units higher.
			// Keep it as the maximum seen. See Sync().
			if (IsSeqHigherThan(output, this->maxOutput))
			{
				this->maxOutput = output;
			}
		}

		ClearDropped();

		return true;
	}

	template<typename T, uint8_t N>
	T SeqManager<T, N>::GetMaxInput() const
	{
		return this->maxInput;
	}

	template<typename T, uint8_t N>
	T SeqManager<T, N>::GetMaxOutput() const
	{
		return this->maxOutput;
	}

	/*
	 * Delete droped inputs greater than maxInput that belong to a previous
	 * cycle.
	 */
	template<typename T, uint8_t N>
	void SeqManager<T, N>::ClearDropped()
	{
		// Cleanup dropped values.
		if (this->dropped.empty())
		{
			return;
		}

		const size_t threshold           = (this->maxInput + MaxValue / 3) & MaxValue;
		const size_t previousDroppedSize = this->dropped.size();
		const auto it1                   = this->dropped.upper_bound(this->maxInput);
		const auto it2                   = this->dropped.lower_bound(threshold);

		// There is no dropped value greater than this->maxInput.
		if (it1 == this->dropped.end())
		{
			return;
		}

		// There is a single value in the range.
		if (it1 == it2)
		{
			this->dropped.erase(it1);
		}
		// There are many values in the range.
		else
		{
			// Measure the distance of it1 and it2 to the beggining of dropped.
			auto distanceIt1 = std::distance(this->dropped.begin(), it1);
			auto distanceIt2 = std::distance(this->dropped.begin(), it2);

			// it2 goes out of range, only it1 is within the range.
			if (distanceIt2 < distanceIt1)
			{
				this->dropped.erase(it1);
			}
			else
			{
				this->dropped.erase(it1, it2);
			}
		}

		// Adapt base.
		this->base = (this->base - (previousDroppedSize - this->dropped.size())) & MaxValue;
	}

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;
	template class SeqManager<uint8_t, 3>; // For testing.
	template class SeqManager<uint16_t>;
	template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
	template class SeqManager<uint32_t>;

} // namespace RTC
