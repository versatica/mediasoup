#define MS_CLASS "RTC::SeqManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SeqManager.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <iterator>

namespace RTC
{
	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqLowerThan::operator()(T lhs, T rhs) const
	{
		return Utils::Number<T, N>::IsLowerThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqHigherThan::operator()(T lhs, T rhs) const
	{
		return Utils::Number<T, N>::IsHigherThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqHigherThan(T lhs, T rhs)
	{
		return Utils::Number<T, N>::IsHigherThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqLowerThan(T lhs, T rhs)
	{
		return Utils::Number<T, N>::IsLowerThan(lhs, rhs);
	}

	template<typename T, uint8_t N>
	SeqManager<T, N>::SeqManager(T initialOutput) : initialOutput(initialOutput)
	{
		MS_TRACE();
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Sync(T input)
	{
		MS_TRACE();

		// Update base.
		this->base = (this->maxOutput - input) & SeqManager::MaxValue;

		// Update maxInput.
		this->maxInput = input;

		// Clear dropped set.
		this->dropped.clear();
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Drop(T input)
	{
		MS_TRACE();

		// Mark as dropped if 'input' is higher than anyone already processed.
		if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
		{
			this->maxInput = input;
			// Insert input in the last position.
			// Explicitly indicate insert() to add the input at the end, which is
			// more performant.
			this->dropped.insert(this->dropped.end(), input);

			ClearDropped();
		}
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::Input(T input, T& output)
	{
		MS_TRACE();

		auto base = this->base;

		// No dropped inputs to consider.
		if (this->dropped.empty())
		{
			goto done;
		}
		// Dropped inputs present, cleanup and update base.
		else
		{
			// Set 'maxInput' here if needed before calling ClearDropped().
			if (this->started && SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
			{
				this->maxInput = input;
			}

			ClearDropped();

			base = this->base;
		}

		// No dropped inputs to consider after cleanup.
		if (this->dropped.empty())
		{
			goto done;
		}
		// This input was dropped.
		else if (this->dropped.find(input) != this->dropped.end())
		{
			MS_DEBUG_DEV("trying to send a dropped input");

			return false;
		}
		// There are dropped inputs, calculate 'base' for this input.
		else
		{
			auto droppedCount = this->dropped.size();

			// Get the first dropped input which is higher than or equal 'input'.
			auto it = this->dropped.lower_bound(input);

			droppedCount -= std::distance(it, this->dropped.end());
			base = (this->base - droppedCount) & SeqManager::MaxValue;
		}

	done:
		output = (input + base) & SeqManager::MaxValue;

		if (!this->started)
		{
			this->started   = true;
			this->maxInput  = input;
			this->maxOutput = output;
		}
		else
		{
			// New input is higher than the maximum seen.
			if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
			{
				this->maxInput = input;
			}

			// New output is higher than the maximum seen.
			if (SeqManager<T, N>::IsSeqHigherThan(output, this->maxOutput))
			{
				this->maxOutput = output;
			}
		}

		output = (output + this->initialOutput) & SeqManager::MaxValue;

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
	 * Delete droped inputs greater than maxInput, which belong to a previous
	 * cycle.
	 */
	template<typename T, uint8_t N>
	void SeqManager<T, N>::ClearDropped()
	{
		MS_TRACE();

		// Cleanup dropped values.
		if (this->dropped.empty())
		{
			return;
		}

		const size_t previousDroppedSize = this->dropped.size();

		for (auto it = this->dropped.begin(); it != this->dropped.end();)
		{
			auto value = *it;

			if (SeqManager<T, N>::IsSeqHigherThan(value, this->maxInput))
			{
				it = this->dropped.erase(it);
			}
			else
			{
				break;
			}
		}

		// Adapt base.
		this->base = (this->base - (previousDroppedSize - this->dropped.size())) & SeqManager::MaxValue;
	}

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;      // For codecs.
	template class SeqManager<uint8_t, 3>;   // For testing.
	template class SeqManager<uint16_t>;     // For RTP sequence numbers.
	template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
} // namespace RTC
