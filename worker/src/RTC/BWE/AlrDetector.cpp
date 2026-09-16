#define MS_CLASS "RTC::BWE::AlrDetector"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/AlrDetector.hpp"
#include "Logger.hpp"
#include "RTC/BWE/Utils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		AlrDetector::AlrDetector(SharedInterface* shared) : AlrDetector(shared, AlrDetectorOptions{})
		{
			MS_TRACE();
		}

		AlrDetector::AlrDetector(SharedInterface* shared, AlrDetectorOptions options)
		  : shared(shared),
		    options(options),
		    // What an interval leaves unspent builds up, since not filling the link
		    // is precisely a state that has to accumulate to be noticed.
		    alrBudget(/*initialTargetBitrate*/ 0, /*canBuildUpUnderuse*/ true)
		{
			MS_TRACE();
		}

		void AlrDetector::OnBytesSent(int64_t bytesSent, int64_t sentAtUs)
		{
			MS_TRACE();

			// How long those bytes took to go out is unknown until there is a previous
			// send to measure them against.
			if (!this->lastSentAtUs.has_value())
			{
				this->lastSentAtUs = sentAtUs;

				return;
			}

			const int64_t deltaTimeUs = sentAtUs - this->lastSentAtUs.value();

			this->lastSentAtUs = sentAtUs;

			this->alrBudget.UseBudget(bytesSent);
			this->alrBudget.IncreaseBudget(deltaTimeUs);

			const double budgetRatio = this->alrBudget.GetBudgetRatio();

			if (budgetRatio > this->options.startBudgetLevelRatio && !this->alrStartTimeUs.has_value())
			{
				// The instant this state began is what the rest of the module reasons
				// about, so it's the current one and not that of the send that revealed
				// it.
				this->alrStartTimeUs = this->shared->GetTimeUs();

				MS_DEBUG_DEV("sender is not filling the link [budgetRatio:%f]", budgetRatio);
			}
			else if (budgetRatio < this->options.stopBudgetLevelRatio && this->alrStartTimeUs.has_value())
			{
				this->alrStartTimeUs.reset();

				MS_DEBUG_DEV("sender is filling the link again [budgetRatio:%f]", budgetRatio);
			}
		}

		void AlrDetector::SetEstimatedBitrate(int64_t bitrate)
		{
			MS_TRACE();

			// NOTE: The target this comes from is clamped to the minimum bitrate of
			// the congestion controller, so anything else is a programming error.
			MS_ASSERT(bitrate > 0, "bitrate must be positive [bitrate:%" PRIi64 "]", bitrate);

			this->alrBudget.SetTargetBitrate(
			  Utils::ApplyBitrateFactor(bitrate, this->options.bandwidthUsageRatio));
		}
	} // namespace BWE
} // namespace RTC
