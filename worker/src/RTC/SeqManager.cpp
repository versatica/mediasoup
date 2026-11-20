#define MS_CLASS "RTC::SeqManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SeqManager.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <iterator> // std::ranges::distance()

namespace RTC
{
	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqLowerThan::operator()(T lhs, T rhs) const
	{
		MS_TRACE();

		return Utils::Number::IsLowerThan<T, N>(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::SeqHigherThan::operator()(T lhs, T rhs) const
	{
		MS_TRACE();

		return Utils::Number::IsHigherThan<T, N>(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqHigherThan(T lhs, T rhs)
	{
		MS_TRACE();

		return Utils::Number::IsHigherThan<T, N>(lhs, rhs);
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::IsSeqLowerThan(T lhs, T rhs)
	{
		MS_TRACE();

		return Utils::Number::IsLowerThan<T, N>(lhs, rhs);
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
		this->base = static_cast<T>(this->maxOutput - input) & SeqManager::MaxValue;

		// Reset maxInput and maxForwarded to the new reference point.
		this->maxInput     = input;
		this->maxForwarded = input;

		// Clear dropped set.
		this->dropped.clear();
		this->droppedStart = 0;
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Drop(T input)
	{
		MS_TRACE();

		// 'input' is higher than any input seen, hence also higher than the highest
		// forwarded one: make it the highest input and add it to 'dropped'.
		if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
		{
			this->maxInput = input;

			// NOTE: Must clear before inserting, see 'ClearDropped()'.
			ClearDropped();
			AddDropped(input);
		}
		// An out of order drop, account it only if it is higher than the highest
		// forwarded input. Accounting a drop lower than an already forwarded input
		// would move the output of the next inputs back, since every dropped input
		// lowers the output of the inputs above it.
		// NOTE: This check cannot replace the one above. 'maxForwarded' may lag
		// 'maxInput' by more than half a cycle, and 'IsSeqHigherThan()' would then
		// not see 'input' above it.
		else if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxForwarded))
		{
			// NOTE: Must clear before inserting, see 'ClearDropped()'.
			ClearDropped();
			AddDropped(input);
		}
	}

	template<typename T, uint8_t N>
	bool SeqManager<T, N>::Input(T input, T& output)
	{
		MS_TRACE();

		auto base = this->base;

		// No dropped inputs to consider.
		if (DroppedSize() == 0)
		{
			goto done;
		}
		// Dropped inputs present, cleanup and update base.
		else
		{
			// Set 'maxInput' and 'maxForwarded' before clearing dropped inputs.
			if (this->started && SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
			{
				this->maxInput     = input;
				this->maxForwarded = input;
			}

			ClearDropped();

			base = this->base;
		}

		// No dropped inputs to consider after cleanup.
		if (DroppedSize() == 0)
		{
			goto done;
		}
		else
		{
			const SeqLowerThan seqLowerThan;
			const auto droppedBegin = this->dropped.begin() + this->droppedStart;
			const auto it =
			  std::ranges::lower_bound(droppedBegin, this->dropped.end(), input, seqLowerThan);

			if (it != this->dropped.end() && *it == input)
			{
				MS_DEBUG_DEV("trying to send a dropped input");

				return false;
			}

			// There are dropped inputs, calculate 'base' for this input.
			const auto droppedCount = std::ranges::distance(droppedBegin, it);

			base = static_cast<T>(this->base - droppedCount) & SeqManager::MaxValue;
		}

	done:
		output = static_cast<T>(input + base) & SeqManager::MaxValue;

		if (!this->started)
		{
			this->started      = true;
			this->maxInput     = input;
			this->maxForwarded = input;
			this->maxOutput    = output;
		}
		else
		{
			// New input is higher than the maximum seen.
			if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
			{
				this->maxInput     = input;
				this->maxForwarded = input;
			}
			// New input is the highest forwarded one.
			else if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxForwarded))
			{
				this->maxForwarded = input;
			}

			// New output is higher than the maximum seen.
			if (SeqManager<T, N>::IsSeqHigherThan(output, this->maxOutput))
			{
				this->maxOutput = output;
			}
		}

		output = static_cast<T>(output + this->initialOutput) & SeqManager::MaxValue;

		return true;
	}

	template<typename T, uint8_t N>
	T SeqManager<T, N>::GetMaxInput() const
	{
		MS_TRACE();

		return this->maxInput;
	}

	template<typename T, uint8_t N>
	T SeqManager<T, N>::GetMaxOutput() const
	{
		MS_TRACE();

		// 'maxOutput' is stored in the offset-less space (as 'base' and comparisons
		// use it), so apply 'initialOutput' here, just like `Input()` does before
		// returning the output to the caller.
		return static_cast<T>(this->maxOutput + this->initialOutput) & SeqManager::MaxValue;
	}

	/*
	 * Insert 'input' into 'dropped', in sorted order, if not present yet.
	 */
	template<typename T, uint8_t N>
	void SeqManager<T, N>::AddDropped(T input)
	{
		MS_TRACE();

		const SeqLowerThan seqLowerThan;
		const auto it = std::ranges::lower_bound(
		  this->dropped.begin() + this->droppedStart, this->dropped.end(), input, seqLowerThan);

		if (it == this->dropped.end() || *it != input)
		{
			this->dropped.insert(it, input);
		}
	}

	/*
	 * Consume dropped inputs higher than 'maxInput', which belong to a previous
	 * cycle, accounting them in 'base'.
	 *
	 * They are not erased as they expire. Erasing from the front of a vector
	 * shifts every remaining entry, and 'dropped' holds up to half a sequence
	 * cycle, so that would mean a full window memmove per input. They are consumed
	 * by advancing 'droppedStart' instead, and the consumed prefix is erased in one
	 * go once it is at least as large as the pending range, which makes removal
	 * O(1) amortized.
	 *
	 * This must always be called before AddDropped().
	 */
	template<typename T, uint8_t N>
	void SeqManager<T, N>::ClearDropped()
	{
		MS_TRACE();

		size_t removed{ 0 };

		// Consume entries belonging to a previous cycle.
		while (this->droppedStart < this->dropped.size() &&
		       // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		       SeqManager<T, N>::IsSeqHigherThan(this->dropped[this->droppedStart], this->maxInput))
		{
			++this->droppedStart;
			++removed;
		}

		if (removed != 0)
		{
			// Adapt base.
			this->base = (this->base - removed) & SeqManager::MaxValue;
		}

		// Erase the consumed prefix once it is at least as large as the pending
		// range, hence at most half a sequence cycle.
		if (this->droppedStart != 0 && this->droppedStart >= DroppedSize())
		{
			this->dropped.erase(this->dropped.begin(), this->dropped.begin() + this->droppedStart);

			this->droppedStart = 0;
		}
	}

	template<typename T, uint8_t N>
	size_t SeqManager<T, N>::DroppedSize() const
	{
		MS_TRACE();

		return this->dropped.size() - this->droppedStart;
	}

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;      // For codecs.
	template class SeqManager<uint8_t, 3>;   // For testing.
	template class SeqManager<uint16_t>;     // For RTP sequence numbers.
	template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
} // namespace RTC
