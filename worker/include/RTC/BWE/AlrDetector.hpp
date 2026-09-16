#ifndef MS_RTC_BWE_ALR_DETECTOR_HPP
#define MS_RTC_BWE_ALR_DETECTOR_HPP

#include "common.hpp"
#include "RTC/BWE/IntervalBudget.hpp"
#include "SharedInterface.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Tells whether the sender is not sending enough to fill the link.
		 *
		 * While that is the case nothing measured says anything about the capacity
		 * of the link, because what arrives is only what was sent: the delay doesn't
		 * grow, no packet is lost, and the bitrate that gets acknowledged is the one
		 * chosen rather than the one the link can carry. So the estimate cannot grow
		 * on its own, and the only way out is to probe.
		 *
		 * The question is answered by giving the sender a budget that refills at a
		 * fraction of the estimated bitrate and is spent as bytes go out. A budget
		 * that keeps growing means the sender is not using what it was given.
		 *
		 * @remarks
		 * - For a browser this is an odd situation, but for an SFU whose consumers
		 *   sit on low layers it is the normal state.
		 */
		class AlrDetector
		{
		public:
			struct AlrDetectorOptions
			{
				/**
				 * Fraction of the estimated bitrate the sender is expected to be using.
				 * The budget refills at that fraction rather than at the whole estimate,
				 * so that staying somewhat below it doesn't count as not filling the
				 * link.
				 */
				double bandwidthUsageRatio{ 0.65 };
				/**
				 * Level the budget has to grow past for the sender to be taken as not
				 * filling the link.
				 */
				double startBudgetLevelRatio{ 0.80 };
				/**
				 * Level the budget has to fall back below for it to stop being taken as
				 * such. It is lower than the one above so that a short burst doesn't end
				 * the state.
				 */
				double stopBudgetLevelRatio{ 0.50 };
			};

		public:
			explicit AlrDetector(SharedInterface* shared);

			AlrDetector(SharedInterface* shared, AlrDetectorOptions options);

			/**
			 * Feed bytes that have just left towards the network.
			 *
			 * @param sentAtUs - Instant at which they left, which is what the span of
			 *   time they were sent over is measured with.
			 */
			void OnBytesSent(int64_t bytesSent, int64_t sentAtUs);

			/**
			 * Bitrate the sender is expected to be able to use (bps).
			 */
			void SetEstimatedBitrate(int64_t bitrate);

			/**
			 * Instant at which the sender started not filling the link, or no value
			 * while it is filling it.
			 */
			std::optional<int64_t> GetAlrStartTimeUs() const
			{
				return this->alrStartTimeUs;
			}

		private:
			// Passed by argument.
			SharedInterface* shared{ nullptr };
			const AlrDetectorOptions options;
			// Others.
			// Budget of what the sender was expected to use, whose level is the whole
			// signal this produces.
			IntervalBudget alrBudget;
			// Instant of the latest send, which is what the span of the next one is
			// measured against.
			std::optional<int64_t> lastSentAtUs;
			std::optional<int64_t> alrStartTimeUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
