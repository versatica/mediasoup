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
	T SeqManager<T, N>::Delta(const T lhs, const T rhs)
	{
		T value = (lhs > rhs) ? (lhs - rhs) : (MaxValue - rhs + lhs);

		return value & MaxValue;
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
			// Delete dropped inputs older than input - MaxValue/2.
			size_t droppedCount    = this->dropped.size();
			const size_t threshold = (input - MaxValue / 2) & MaxValue;
			auto it                = this->dropped.lower_bound(threshold);
			this->dropped.erase(this->dropped.begin(), it);
			this->base = (this->base - (droppedCount - this->dropped.size())) & MaxValue;

			// Count dropped entries before 'input' in order to adapt the base.
			droppedCount = this->dropped.size();
			it           = this->dropped.lower_bound(input);

			if (it != this->dropped.end())
			{
				// Check whether this input was dropped.
				if (*it == input)
				{
					MS_DEBUG_DEV("trying to send a dropped input");

					return false;
				}

				droppedCount -= std::distance(it, this->dropped.end());
			}

			base = (this->base - droppedCount) & MaxValue;
		}

		output = (input + base) & MaxValue;

		T idelta = SeqManager<T, N>::Delta(input, this->maxInput);
		T odelta = SeqManager<T, N>::Delta(output, this->maxOutput);

		// New input is higher than the maximum seen. But less than acceptable units higher.
		// Keep it as the maximum seen. See Drop().
		if (idelta < MaxValue / 2)
			this->maxInput = input;

		// New output is higher than the maximum seen. But less than acceptable units higher.
		// Keep it as the maximum seen. See Sync().
		if (odelta < MaxValue / 2)
			this->maxOutput = output;

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

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;
	template class SeqManager<uint16_t>;
	template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
	template class SeqManager<uint32_t>;

} // namespace RTC
