#define MS_CLASS "RTC::SeqManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SeqManager.hpp"
#include "Logger.hpp"
#include <iterator>

namespace RTC
{
	template<typename T>
	bool SeqManager<T>::SeqLowerThan::operator()(const T lhs, const T rhs) const
	{
		return ((rhs > lhs) && (rhs - lhs <= MaxValue / 2)) ||
		       ((lhs > rhs) && (lhs - rhs > MaxValue / 2));
	}

	template<typename T>
	bool SeqManager<T>::SeqHigherThan::operator()(const T lhs, const T rhs) const
	{
		return ((lhs > rhs) && (lhs - rhs <= MaxValue / 2)) ||
		       ((rhs > lhs) && (rhs - lhs > MaxValue / 2));
	}

	template<typename T>
	const typename SeqManager<T>::SeqLowerThan SeqManager<T>::isSeqLowerThan{};

	template<typename T>
	const typename SeqManager<T>::SeqHigherThan SeqManager<T>::isSeqHigherThan{};

	template<typename T>
	bool SeqManager<T>::IsSeqLowerThan(const T lhs, const T rhs)
	{
		return isSeqLowerThan(lhs, rhs);
	}

	template<typename T>
	bool SeqManager<T>::IsSeqHigherThan(const T lhs, const T rhs)
	{
		return isSeqHigherThan(lhs, rhs);
	}

	template<typename T>
	void SeqManager<T>::Sync(T input)
	{
		// Update base.
		this->base = this->maxOutput - input;

		// Update maxInput.
		this->maxInput = input;

		// Clear dropped set.
		this->dropped.clear();
	}

	template<typename T>
	void SeqManager<T>::Drop(T input)
	{
		// Mark as dropped if 'input' is higher than anyone already processed.
		if (SeqManager<T>::IsSeqHigherThan(input, this->maxInput))
		{
			this->dropped.insert(input);
		}
	}

	template<typename T>
	void SeqManager<T>::Offset(T offset)
	{
		this->base += offset;
	}

	template<typename T>
	bool SeqManager<T>::Input(const T input, T& output)
	{
		auto base = this->base;

		// There are dropped inputs. Synchronize.
		if (!this->dropped.empty())
		{
			// Delete dropped inputs older than input - MaxValue/2.
			size_t droppedCount = this->dropped.size();
			auto it             = this->dropped.lower_bound(input - MaxValue / 2);

			this->dropped.erase(this->dropped.begin(), it);
			this->base -= (droppedCount - this->dropped.size());

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

			base = this->base - droppedCount;
		}

		output = input + base;

		T idelta = input - this->maxInput;
		T odelta = output - this->maxOutput;

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

	template<typename T>
	T SeqManager<T>::GetMaxInput() const
	{
		return this->maxInput;
	}

	template<typename T>
	T SeqManager<T>::GetMaxOutput() const
	{
		return this->maxOutput;
	}

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;
	template class SeqManager<uint16_t>;
	template class SeqManager<uint32_t>;

} // namespace RTC
