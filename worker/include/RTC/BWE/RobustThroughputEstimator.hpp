#ifndef MS_RTC_BWE_ROBUST_THROUGHPUT_ESTIMATOR_HPP
#define MS_RTC_BWE_ROBUST_THROUGHPUT_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/BWE/AcknowledgedBitrateEstimatorInterface.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include <deque>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Measures the acknowledged bitrate over a sliding window of the packets the
		 * receiver reported.
		 *
		 * The estimate is the smaller of the send rate and the receive rate observed
		 * over that window, so that it never claims more than what was actually put
		 * on the wire. The largest gap between arrivals is replaced by the second
		 * largest one before dividing, which is what keeps a short pause followed by
		 * a burst of delayed packets from collapsing the estimate.
		 */
		class RobustThroughputEstimator : public AcknowledgedBitrateEstimatorInterface
		{
		public:
			/**
			 * @remarks
			 * - Every constraint documented below is checked by the constructor and
			 *   aborts when it doesn't hold. These options are set from C++ alone,
			 *   never from the network nor from the API, so breaking one of them is a
			 *   programming error.
			 */
			struct RobustThroughputEstimatorOptions
			{
				/**
				 * Smallest number of packets the window holds.
				 *
				 * @remarks
				 * - It must be between 10 and 1000.
				 */
				size_t windowPackets{ 20 };
				/**
				 * Largest number of packets the window holds, so that a high bitrate
				 * doesn't make it grow without bound.
				 *
				 * @remarks
				 * - It must be between 10 and 1000, and never below `windowPackets`.
				 */
				size_t maxWindowPackets{ 500 };
				/**
				 * Smallest duration the window covers, so that a low bitrate still gets
				 * enough packets to measure.
				 *
				 * @remarks
				 * - It must be between 100 and 3000 ms, and never above
				 *   `maxWindowDurationUs`.
				 */
				int64_t minWindowDurationUs{ 1000 * 1000 };
				/**
				 * Largest duration the window covers, so that very old packets don't
				 * weigh on the estimate after sending was paused.
				 *
				 * @remarks
				 * - It must be between 1 and 15 s.
				 */
				int64_t maxWindowDurationUs{ 5 * 1000 * 1000 };
				/**
				 * Number of packets the window needs before it produces an estimate.
				 *
				 * @remarks
				 * - It must be between 10 and 1000, and never above `windowPackets`,
				 *   since a window that requires more packets than it holds would reach
				 *   the sending rate with no send time to compute it from.
				 */
				size_t requiredPackets{ 10 };
				/**
				 * Weight given to the data that was sent but is not covered by any
				 * feedback.
				 *
				 * @remarks
				 * - Set it to 0 when audio is not included in the allocation, and to 1
				 *   when it is included in the allocation but not in the estimation.
				 *   Its value is irrelevant once every packet is tracked.
				 * - It must be between 0 and 1.
				 */
				double unackedWeight{ 1.0 };
			};

		public:
			RobustThroughputEstimator();

			explicit RobustThroughputEstimator(RobustThroughputEstimatorOptions options);

			void IncomingPacketFeedbackVector(const std::vector<Types::PacketResult>& packetResults) override;

			std::optional<int64_t> GetBitrate() const override;

			void SetAlr(bool /*inAlr*/) override
			{
			}

			void SetAlrEndedTimeUs(int64_t /*alrEndedTimeUs*/) override
			{
			}

		private:
			/**
			 * Whether the oldest packet of the window no longer belongs to it.
			 */
			bool IsFirstPacketOutsideWindow() const;

		private:
			const RobustThroughputEstimatorOptions options;
			std::deque<Types::PacketResult> window;
			std::optional<int64_t> latestDiscardedSendTimeUs;
		};
	} // namespace BWE
} // namespace RTC

#endif
