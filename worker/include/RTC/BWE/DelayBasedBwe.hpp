#ifndef MS_RTC_BWE_DELAY_BASED_BWE_HPP
#define MS_RTC_BWE_DELAY_BASED_BWE_HPP

#include "common.hpp"
#include "RTC/BWE/AimdRateControl.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/InterArrivalDelta.hpp"
#include "RTC/BWE/TrendlineEstimator.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Estimates the available bitrate out of the delay between the packets that
		 * were sent and the feedback reporting when they arrived.
		 *
		 * It's the glue of the delay based path: the packets of each feedback are
		 * grouped into send bursts, the delay between consecutive groups feeds the
		 * trendline detector, and what the detector says about the network drives
		 * the rate control.
		 */
		class DelayBasedBwe
		{
		public:
			struct DelayBasedBweOptions
			{
				/**
				 * Whether audio and video get a delay detector of their own instead of
				 * sharing one.
				 */
				bool separateAudioPackets{ false };
				/**
				 * Number of audio packets without a video one after which the audio
				 * detector becomes the active one.
				 *
				 * @remarks
				 * - Only used when `separateAudioPackets` is set.
				 */
				int64_t separateAudioPacketThreshold{ 10 };
				/**
				 * Time without a video packet after which the audio detector becomes
				 * the active one.
				 *
				 * @remarks
				 * - Only used when `separateAudioPackets` is set.
				 */
				int64_t separateAudioTimeThresholdUs{ 1000 * 1000 };
			};

			struct Result
			{
				/**
				 * Whether the target bitrate was recomputed.
				 */
				bool updated{ false };
				/**
				 * Whether the target bitrate comes from the bitrate measured by a
				 * probe.
				 */
				bool probe{ false };
				/**
				 * Target bitrate (bps).
				 */
				int64_t targetBitrate{ 0 };
				/**
				 * Whether the detector went back to normal after having been
				 * underusing.
				 */
				bool recoveredFromOveruse{ false };
				/**
				 * What the delay based detector says about the network.
				 */
				Types::BandwidthUsage delayDetectorState{ Types::BandwidthUsage::NORMAL };
			};

		public:
			DelayBasedBwe();

			explicit DelayBasedBwe(DelayBasedBweOptions options);

			/**
			 * Feed a whole feedback message.
			 *
			 * @param feedback - What the receiver reported about the packets sent so
			 *   far.
			 * @param ackedBitrate - Bitrate acknowledged by the receiver (bps), or no
			 *   value if it could not be measured.
			 * @param probeBitrate - Bitrate measured by a probe (bps), or no value if
			 *   no probe was completed.
			 * @param networkEstimate - Estimate of the capacity of the link, used to
			 *   bound the rate control.
			 * @param inAlr - Whether the sender is in an application limited region.
			 */
			Result IncomingPacketFeedbackVector(
			  const Types::TransportPacketsFeedback& feedback,
			  std::optional<int64_t> ackedBitrate,
			  std::optional<int64_t> probeBitrate,
			  const std::optional<Types::NetworkStateEstimate>& networkEstimate,
			  bool inAlr);

			void OnRttUpdate(int64_t avgRttUs);

			/**
			 * @param startBitrate - Bitrate to start from (bps).
			 */
			void SetStartBitrate(int64_t startBitrate);

			/**
			 * @param minBitrate - Bitrate the target is never taken below (bps).
			 */
			void SetMinBitrate(int64_t minBitrate);

			/**
			 * Current target bitrate of the rate control (bps), or no value if there
			 * is no valid estimate yet.
			 *
			 * @remarks
			 * - Unlike `GetLastEstimate()`, this is what the rate control holds right
			 *   now rather than the latest value that was reported.
			 */
			std::optional<int64_t> GetLatestEstimate() const;

			/**
			 * Latest target bitrate that was reported (bps).
			 */
			int64_t GetLastEstimate() const
			{
				return this->prevBitrate;
			}

			/**
			 * Latest state of the delay based detector that was reported.
			 */
			Types::BandwidthUsage GetLastState() const
			{
				return this->prevState;
			}

		private:
			void IncomingPacketFeedback(const Types::PacketResult& packetResult, int64_t atTimeUs);

			Result MaybeUpdateEstimate(
			  std::optional<int64_t> ackedBitrate,
			  std::optional<int64_t> probeBitrate,
			  bool recoveredFromOveruse,
			  int64_t atTimeUs);

			/**
			 * Recompute the target bitrate.
			 *
			 * @returns Whether there is a valid estimate.
			 */
			bool UpdateEstimate(int64_t atTimeUs, std::optional<int64_t> ackedBitrate, int64_t& targetBitrate);

		private:
			const DelayBasedBweOptions options;
			int64_t audioPacketsSinceLastVideo{ 0 };
			std::optional<int64_t> lastVideoPacketRecvTimeUs;
			std::optional<InterArrivalDelta> videoInterArrivalDelta;
			std::optional<InterArrivalDelta> audioInterArrivalDelta;
			std::optional<TrendlineEstimator> videoDelayDetector;
			std::optional<TrendlineEstimator> audioDelayDetector;
			TrendlineEstimator* activeDelayDetector{ nullptr };
			std::optional<int64_t> lastSeenPacketUs;
			AimdRateControl rateControl;
			int64_t prevBitrate{ 0 };
			Types::BandwidthUsage prevState{ Types::BandwidthUsage::NORMAL };
		};
	} // namespace BWE
} // namespace RTC

#endif
