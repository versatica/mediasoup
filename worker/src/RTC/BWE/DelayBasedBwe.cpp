#define MS_CLASS "RTC::BWE::DelayBasedBwe"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/DelayBasedBwe.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Time without any packet after which everything learnt about the stream is
		// dropped.
		static constexpr int64_t StreamTimeOutUs{ 2 * 1000 * 1000 };
		// A send time group holds every packet sent within this time of the first
		// packet of the group.
		static constexpr int64_t SendTimeGroupLengthUs{ 5 * 1000 };

		/* Instance methods. */

		DelayBasedBwe::DelayBasedBwe() : DelayBasedBwe(DelayBasedBweOptions{})
		{
			MS_TRACE();
		}

		DelayBasedBwe::DelayBasedBwe(DelayBasedBweOptions options)
		  : options(options),
		    videoDelayDetector(std::in_place),
		    audioDelayDetector(std::in_place),
		    activeDelayDetector(std::addressof(this->videoDelayDetector.value())),
		    rateControl(AimdRateControl::AimdRateControlOptions{ .sendSide = true })
		{
			MS_TRACE();
		}

		DelayBasedBwe::Result DelayBasedBwe::IncomingPacketFeedbackVector(
		  const Types::TransportPacketsFeedback& feedback,
		  std::optional<int64_t> ackedBitrate,
		  std::optional<int64_t> probeBitrate,
		  const std::optional<Types::NetworkStateEstimate>& networkEstimate,
		  bool inAlr)
		{
			MS_TRACE();

			const auto packetResults = feedback.SortedByReceiveTime();

			// NOTE: An empty vector here likely means that every ack arrived too late
			// and that the send time history had already timed out.
			if (packetResults.empty())
			{
				MS_WARN_TAG(bwe, "very late feedback received");

				return {};
			}

			bool recoveredFromOveruse{ false };
			auto prevDetectorState = this->activeDelayDetector->GetState();

			for (const auto& packetResult : packetResults)
			{
				IncomingPacketFeedback(packetResult, feedback.feedbackTimeUs);

				if (
				  prevDetectorState == Types::BandwidthUsage::UNDERUSING &&
				  this->activeDelayDetector->GetState() == Types::BandwidthUsage::NORMAL)
				{
					recoveredFromOveruse = true;
				}

				prevDetectorState = this->activeDelayDetector->GetState();
			}

			this->rateControl.SetInApplicationLimitedRegion(inAlr);
			this->rateControl.SetNetworkStateEstimate(networkEstimate);

			return MaybeUpdateEstimate(
			  ackedBitrate, probeBitrate, recoveredFromOveruse, feedback.feedbackTimeUs);
		}

		void DelayBasedBwe::IncomingPacketFeedback(const Types::PacketResult& packetResult, int64_t atTimeUs)
		{
			MS_TRACE();

			// NOTE: Safe since only the packets that were received reach this method,
			// as the caller iterates over `SortedByReceiveTime()`.
			MS_ASSERT(packetResult.IsReceived(), "packet was not received");

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t receiveTimeUs = packetResult.receiveTimeUs.value();

			// Reset if the stream has timed out, since nothing measured before such a
			// silence still describes this link.
			if (!this->lastSeenPacketUs.has_value() || atTimeUs - this->lastSeenPacketUs.value() > StreamTimeOutUs)
			{
				this->videoInterArrivalDelta.emplace(SendTimeGroupLengthUs);
				this->audioInterArrivalDelta.emplace(SendTimeGroupLengthUs);
				this->videoDelayDetector.emplace();
				this->audioDelayDetector.emplace();

				this->activeDelayDetector = std::addressof(this->videoDelayDetector.value());
			}

			this->lastSeenPacketUs = atTimeUs;

			// As an alternative to ignoring small packets, audio and video can be given
			// a delay detector of their own.
			auto* delayDetectorForPacket = std::addressof(this->videoDelayDetector.value());

			if (this->options.separateAudioPackets)
			{
				if (packetResult.sentPacket.audio)
				{
					delayDetectorForPacket = std::addressof(this->audioDelayDetector.value());

					this->audioPacketsSinceLastVideo++;

					// Video has been silent for long enough that the audio detector is a
					// better description of the network than the stale video one.
					if (
					  this->audioPacketsSinceLastVideo > this->options.separateAudioPacketThreshold &&
					  (!this->lastVideoPacketRecvTimeUs.has_value() ||
						 receiveTimeUs - this->lastVideoPacketRecvTimeUs.value() >
						   this->options.separateAudioTimeThresholdUs))
					{
						this->activeDelayDetector = std::addressof(this->audioDelayDetector.value());
					}
				}
				else
				{
					this->audioPacketsSinceLastVideo = 0;

					this->lastVideoPacketRecvTimeUs =
					  std::max(this->lastVideoPacketRecvTimeUs.value_or(receiveTimeUs), receiveTimeUs);
					this->activeDelayDetector = std::addressof(this->videoDelayDetector.value());
				}
			}

			auto& interArrivalForPacket =
			  (this->options.separateAudioPackets && packetResult.sentPacket.audio)
			    ? this->audioInterArrivalDelta.value()
			    : this->videoInterArrivalDelta.value();

			const auto deltas = interArrivalForPacket.ComputeDeltas(
			  packetResult.sentPacket.sendTimeUs, receiveTimeUs, atTimeUs, packetResult.sentPacket.size);

			if (deltas.has_value())
			{
				delayDetectorForPacket->Update(
				  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				  deltas.value().sendDeltaUs,
				  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				  deltas.value().arrivalDeltaUs,
				  receiveTimeUs);
			}
		}

		DelayBasedBwe::Result DelayBasedBwe::MaybeUpdateEstimate(
		  std::optional<int64_t> ackedBitrate,
		  std::optional<int64_t> probeBitrate,
		  bool recoveredFromOveruse,
		  int64_t atTimeUs)
		{
			MS_TRACE();

			Result result;

			if (this->activeDelayDetector->GetState() == Types::BandwidthUsage::OVERUSING)
			{
				if (
				  ackedBitrate.has_value() &&
				  this->rateControl.IsTimeToReduceFurther(atTimeUs, ackedBitrate.value()))
				{
					result.updated = UpdateEstimate(atTimeUs, ackedBitrate, result.targetBitrate);
				}
				else if (
				  !ackedBitrate.has_value() && this->rateControl.IsValidEstimate() &&
				  this->rateControl.IsInitialTimeToReduceFurther(atTimeUs))
				{
					// Overusing before any throughput has been measured, so halve the
					// bitrate instead of aiming at a measurement we don't have.
					this->rateControl.SetEstimate(this->rateControl.GetLatestEstimate() / 2, atTimeUs);

					result.updated       = true;
					result.probe         = false;
					result.targetBitrate = this->rateControl.GetLatestEstimate();
				}
			}
			else
			{
				if (probeBitrate.has_value())
				{
					result.probe   = true;
					result.updated = true;

					this->rateControl.SetEstimate(probeBitrate.value(), atTimeUs);

					result.targetBitrate = this->rateControl.GetLatestEstimate();
				}
				else
				{
					result.updated = UpdateEstimate(atTimeUs, ackedBitrate, result.targetBitrate);
					result.recoveredFromOveruse = recoveredFromOveruse;
				}
			}

			const auto detectorState = this->activeDelayDetector->GetState();

			if ((result.updated && this->prevBitrate != result.targetBitrate) || detectorState != this->prevState)
			{
				this->prevBitrate = result.updated ? result.targetBitrate : this->prevBitrate;
				this->prevState   = detectorState;
			}

			result.delayDetectorState = detectorState;

			return result;
		}

		bool DelayBasedBwe::UpdateEstimate(
		  int64_t atTimeUs, std::optional<int64_t> ackedBitrate, int64_t& targetBitrate)
		{
			MS_TRACE();

			const Types::RateControlInput input{ .bandwidthUsage = this->activeDelayDetector->GetState(),
			                                     .estimatedThroughput = ackedBitrate };

			targetBitrate = this->rateControl.Update(input, atTimeUs);

			return this->rateControl.IsValidEstimate();
		}

		std::optional<int64_t> DelayBasedBwe::GetLatestEstimate() const
		{
			MS_TRACE();

			if (!this->rateControl.IsValidEstimate())
			{
				return std::nullopt;
			}

			return this->rateControl.GetLatestEstimate();
		}

		void DelayBasedBwe::OnRttUpdate(int64_t avgRttUs)
		{
			MS_TRACE();

			this->rateControl.SetRtt(avgRttUs);
		}

		void DelayBasedBwe::SetStartBitrate(int64_t startBitrate)
		{
			MS_TRACE();

			MS_DEBUG_TAG(bwe, "setting start bitrate [bitrate:%" PRIi64 "]", startBitrate);

			this->rateControl.SetStartBitrate(startBitrate);
		}

		void DelayBasedBwe::SetMinBitrate(int64_t minBitrate)
		{
			MS_TRACE();

			this->rateControl.SetMinBitrate(minBitrate);
		}

	} // namespace BWE
} // namespace RTC
