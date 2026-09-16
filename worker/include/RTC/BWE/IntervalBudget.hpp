#ifndef MS_RTC_BWE_INTERVAL_BUDGET_HPP
#define MS_RTC_BWE_INTERVAL_BUDGET_HPP

#include "common.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Tracks how much of a bitrate is being used over a sliding span of time.
		 *
		 * The budget is a number of bytes that refills at a given bitrate and is
		 * spent as bytes are sent, bounded in both directions by what that bitrate
		 * delivers over a fixed window. Its level relative to that bound is the
		 * answer to whether the sender is keeping up with the bitrate it was given:
		 * a full budget means nothing has been sent for a while, and an empty one
		 * means the bitrate is being used whole.
		 *
		 * The bound in the negative direction is what keeps a long burst from
		 * building a debt that would take an unbounded time to pay back.
		 */
		class IntervalBudget
		{
		public:
			explicit IntervalBudget(int64_t initialTargetBitrate);

			/**
			 * @param canBuildUpUnderuse - Whether what an interval leaves unspent is
			 *   available to the next one. When false, an interval that spent nothing
			 *   leaves the budget at just what that one interval refills, instead of
			 *   adding to what was already there.
			 */
			IntervalBudget(int64_t initialTargetBitrate, bool canBuildUpUnderuse);

			/**
			 * Set the bitrate the budget refills at (bps).
			 */
			void SetTargetBitrate(int64_t targetBitrate);

			int64_t GetTargetBitrate() const
			{
				return this->targetBitrate;
			}

			/**
			 * Refill the budget with what the target bitrate delivers over the given
			 * span of time.
			 */
			void IncreaseBudget(int64_t deltaTimeUs);

			/**
			 * Spend the given number of bytes.
			 */
			void UseBudget(int64_t bytes);

			/**
			 * Bytes still available, which is zero while the budget is in debt.
			 */
			int64_t GetBytesRemaining() const;

			/**
			 * Level of the budget as a fraction of what the window holds, from -1
			 * when the debt is at its bound to 1 when the budget is full.
			 */
			double GetBudgetRatio() const;

		private:
			// Passed by argument.
			const bool canBuildUpUnderuse;
			// Others.
			int64_t targetBitrate{ 0 };
			// Bytes the target bitrate delivers over the window, which is what bounds
			// the budget in both directions.
			int64_t maxBytesInBudget{ 0 };
			int64_t bytesRemaining{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
