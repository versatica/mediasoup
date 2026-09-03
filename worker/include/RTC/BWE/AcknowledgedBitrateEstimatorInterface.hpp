#ifndef MS_RTC_BWE_ACKNOWLEDGED_BITRATE_ESTIMATOR_INTERFACE_HPP
#define MS_RTC_BWE_ACKNOWLEDGED_BITRATE_ESTIMATOR_INTERFACE_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Measures the bitrate the receiver is acknowledging, which is what the rate
		 * control compares its target against.
		 */
		class AcknowledgedBitrateEstimatorInterface
		{
		public:
			AcknowledgedBitrateEstimatorInterface() = default;

			AcknowledgedBitrateEstimatorInterface& operator=(
			  const AcknowledgedBitrateEstimatorInterface&) = delete;

			AcknowledgedBitrateEstimatorInterface(const AcknowledgedBitrateEstimatorInterface&) = delete;

			virtual ~AcknowledgedBitrateEstimatorInterface() = default;

		public:
			/**
			 * Feed the packets a feedback reported as received.
			 *
			 * @param packetResults - The reported packets, ordered by arrival time.
			 */
			virtual void IncomingPacketFeedbackVector(
			  const std::vector<Types::PacketResult>& packetResults) = 0;

			/**
			 * Acknowledged bitrate (bps), or no value if it cannot be measured yet.
			 */
			virtual std::optional<int64_t> GetBitrate() const = 0;

			/**
			 * @param inAlr - Whether the sender is in an application limited region.
			 */
			virtual void SetAlr(bool inAlr) = 0;

			/**
			 * @param alrEndedTimeUs - Time at which the sender stopped being in an
			 *   application limited region.
			 */
			virtual void SetAlrEndedTime(int64_t alrEndedTimeUs) = 0;
		};
	} // namespace BWE
} // namespace RTC

#endif
