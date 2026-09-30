#ifndef MS_TEST_RTC_BWE_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_TEST_HELPER_HPP
#define MS_TEST_RTC_BWE_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_TEST_HELPER_HPP

#include "common.hpp"
#include "RTC/BWE/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "test/include/RTC/BWE/helpers/LinkSimulator.hpp"
#include <vector>

namespace bweHelpers
{
	/**
	 * Drives a `RTC::BWE::RemoteBitrateEstimatorAbsSendTime` through the scenarios
	 * its tests are written around: it owns the estimator, feeds it the packets a
	 * simulated link delivers, and keeps the latest estimation it reported.
	 */
	class RemoteBitrateEstimatorAbsSendTimeTestHelper
	  : public RTC::BWE::RemoteBitrateEstimatorAbsSendTime::Listener
	{
	public:
		/**
		 * SSRC of the stream `AddDefaultStream()` adds.
		 */
		static constexpr uint32_t DefaultSsrc{ 1 };

	public:
		RemoteBitrateEstimatorAbsSendTimeTestHelper();

		~RemoteBitrateEstimatorAbsSendTimeTestHelper() override = default;

		RemoteBitrateEstimatorAbsSendTimeTestHelper(const RemoteBitrateEstimatorAbsSendTimeTestHelper&) =
		  delete;
		RemoteBitrateEstimatorAbsSendTimeTestHelper& operator=(
		  const RemoteBitrateEstimatorAbsSendTimeTestHelper&) = delete;

		/**
		 * Turn an instant into the 24 bits of the 'abs-send-time' extension,
		 * rounded upwards.
		 *
		 * @param denom - What to divide the instant by to get whole seconds, so
		 *   1000 for milliseconds.
		 */
		static uint32_t AbsSendTime(int64_t t, int64_t denom);

		/**
		 * Add two of those values together, keeping the result within 24 bits.
		 */
		static uint32_t AddAbsSendTime(uint32_t t1, uint32_t t2);

		void AddDefaultStream();

		/**
		 * Build a packet with the given 'abs-send-time' and feed it to the
		 * estimator as having arrived at `arrivalTimeMs`.
		 */
		void IncomingPacket(
		  uint32_t ssrc,
		  size_t payloadSize,
		  int64_t arrivalTimeMs,
		  uint32_t rtpTimestamp,
		  uint32_t absSendTime);

		/**
		 * Generate a frame at the given bitrate, push it through the link and give
		 * every packet to the estimator.
		 *
		 * @returns Whether the estimation came out below what was being sent, which
		 *   is what an overuse looks like from here.
		 */
		bool GenerateAndProcessFrame(uint32_t ssrc, int64_t bitrateBps);

		/**
		 * Feed frames until the estimation settles above `targetBitrate` or until
		 * `maxNumberOfFrames` have gone by, which is how a scenario gets the
		 * estimator into a steady state before doing anything to it.
		 *
		 * @returns The bitrate reached.
		 */
		int64_t SteadyStateRun(
		  uint32_t ssrc,
		  int maxNumberOfFrames,
		  int64_t startBitrate,
		  int64_t minBitrate,
		  int64_t maxBitrate,
		  int64_t targetBitrate);

		void InitialBehaviorTestHelper(int64_t expectedConvergeBitrate);

		void RateIncreaseReorderingTestHelper(int64_t expectedBitrate);

		void RateIncreaseRtpTimestampsTestHelper(int expectedIterations);

		void CapacityDropTestHelper(
		  int numberOfStreams,
		  bool wrapTimestamp,
		  int64_t expectedBitrateDropDeltaMs,
		  int64_t receiverClockOffsetChangeMs);

		void TestTimestampGroupingTestHelper();

		void TestWrappingHelper(int silenceTimeS);

		/* Pure virtual methods inherited from RemoteBitrateEstimatorAbsSendTime::Listener. */
	public:
		void OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
		  RTC::BWE::RemoteBitrateEstimatorAbsSendTime* remoteBitrateEstimator,
		  const std::vector<uint32_t>& ssrcs,
		  int64_t bitrate) override;

	private:
		// Current instant, which the scenarios move forward themselves since no
		// class of the module reads a clock.
		int64_t nowUs;
		// Added to the arrival time of every packet, so that a scenario can move
		// the receiver's clock mid-run.
		int64_t arrivalTimeOffsetMs{ 0 };
		// Whether the estimation was reported since the latest reset.
		bool updated{ false };
		// Latest estimation reported (bps).
		int64_t latestBitrate{ 0 };
		LinkSimulator linkSimulator;
		RTC::BWE::RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator;
	};
} // namespace bweHelpers

#endif
