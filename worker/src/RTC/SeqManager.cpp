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
		this->base = (this->maxOutput - input) & SeqManager::MaxValue;

		// Update maxInput.
		this->maxInput = input;

		// Clear dropped set.
		this->dropped.clear();
		this->droppedStart = 0;
	}

	template<typename T, uint8_t N>
	void SeqManager<T, N>::Drop(T input)
	{
		MS_TRACE();

		// Mark as dropped if 'input' is higher than anyone already processed.
		if (SeqManager<T, N>::IsSeqHigherThan(input, this->maxInput))
		{
			this->maxInput   = input;
			this->maxDropped = input;

			// NOTE: Must clear before inserting, see 'ClearDropped()'.
			ClearDropped();
			AddDropped(input);
		}
		// Mark as dropped if no input was forwarded after the last dropped one and
		// 'input' is higher than the last forwarded input 'this->maxForwarded'.
		// Allows for properly accounting for out of order drops until an input is forwarded.
		else if (this->maxInput == this->maxDropped && SeqManager<T, N>::IsSeqHigherThan(input, this->maxForwarded))
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
			// Set 'maxInput' here if needed before calling ClearDropped().
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
			const auto it = std::lower_bound(droppedBegin, this->dropped.end(), input, seqLowerThan);

			if (it != this->dropped.end() && *it == input)
			{
				MS_DEBUG_DEV("trying to send a dropped input");

				return false;
			}

			// There are dropped inputs, calculate 'base' for this input.
			auto droppedCount = std::distance(droppedBegin, it);

			base = (this->base - droppedCount) & SeqManager::MaxValue;
		}

	done:
		output = (input + base) & SeqManager::MaxValue;

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
		return (this->maxOutput + this->initialOutput) & SeqManager::MaxValue;
	}

	template<typename T, uint8_t N>
	size_t SeqManager<T, N>::DroppedSize() const
	{
		MS_TRACE();

		return this->dropped.size() - this->droppedStart;
	}

	/*
	 * Insert 'input' into 'dropped', in sorted order, if not present yet.
	 */
	template<typename T, uint8_t N>
	void SeqManager<T, N>::AddDropped(T input)
	{
		MS_TRACE();

		const SeqLowerThan seqLowerThan;
		const auto it = std::lower_bound(
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

	// Explicit instantiation to have all SeqManager definitions in this file.
	template class SeqManager<uint8_t>;      // For codecs.
	template class SeqManager<uint8_t, 3>;   // For testing.
	template class SeqManager<uint16_t>;     // For RTP sequence numbers.
	template class SeqManager<uint16_t, 15>; // For PictureID (15 bits).
} // namespace RTC
