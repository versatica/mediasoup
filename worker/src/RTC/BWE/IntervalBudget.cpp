#define MS_CLASS "RTC::BWE::IntervalBudget"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/IntervalBudget.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Span of time the budget is measured over.
		static constexpr int64_t WindowUs{ 500 * 1000 };

		/* Instance methods. */

		IntervalBudget::IntervalBudget(int64_t initialTargetBitrate)
		  : IntervalBudget(initialTargetBitrate, /*canBuildUpUnderuse*/ false)
		{
			MS_TRACE();
		}

		IntervalBudget::IntervalBudget(int64_t initialTargetBitrate, bool canBuildUpUnderuse)
		  : canBuildUpUnderuse(canBuildUpUnderuse)
		{
			MS_TRACE();

			SetTargetBitrate(initialTargetBitrate);
		}

		void IntervalBudget::SetTargetBitrate(int64_t targetBitrate)
		{
			MS_TRACE();

			this->targetBitrate = targetBitrate;
			// Bits over a second into bytes over the window.
			this->maxBytesInBudget = (this->targetBitrate * WindowUs) / (8 * 1000000);

			// What the budget held doesn't survive a window it no longer fits in.
			this->bytesRemaining =
			  std::clamp(this->bytesRemaining, -this->maxBytesInBudget, this->maxBytesInBudget);
		}

		void IntervalBudget::IncreaseBudget(int64_t deltaTimeUs)
		{
			MS_TRACE();

			// Bits over a second into the bytes that fit in the span given, which is
			// negative when the span is, so that time running backwards takes budget
			// away rather than leaving it untouched.
			const int64_t bytes = (this->targetBitrate * deltaTimeUs) / (8 * 1000000);

			if (this->bytesRemaining < 0 || this->canBuildUpUnderuse)
			{
				// The budget was overspent, so this span makes up for it.
				this->bytesRemaining = std::min(this->bytesRemaining + bytes, this->maxBytesInBudget);
			}
			else
			{
				// What was left unspent is not available anymore.
				this->bytesRemaining = std::min(bytes, this->maxBytesInBudget);
			}
		}

		void IntervalBudget::UseBudget(int64_t bytes)
		{
			MS_TRACE();

			this->bytesRemaining = std::max(this->bytesRemaining - bytes, -this->maxBytesInBudget);
		}

		int64_t IntervalBudget::GetBytesRemaining() const
		{
			MS_TRACE();

			return std::max<int64_t>(0, this->bytesRemaining);
		}

		double IntervalBudget::GetBudgetRatio() const
		{
			MS_TRACE();

			// Nothing can be left over nor owed at no bitrate at all.
			if (this->maxBytesInBudget == 0)
			{
				return 0.0;
			}

			return static_cast<double>(this->bytesRemaining) / static_cast<double>(this->maxBytesInBudget);
		}
	} // namespace BWE
} // namespace RTC
