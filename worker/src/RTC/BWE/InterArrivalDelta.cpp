#define MS_CLASS "RTC::BWE::InterArrivalDelta"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/InterArrivalDelta.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Maximum arrival time delta for a packet to be absorbed into the ongoing
		// burst.
		static constexpr int64_t BurstDeltaThresholdUs{ 5000 };
		// Maximum duration of a burst.
		static constexpr int64_t MaxBurstDurationUs{ 100000 };
		// Number of consecutive reordered groups after which the state is reset.
		static constexpr size_t ReorderedResetThreshold{ 3 };
		// Difference between the arrival time delta and our own time delta after
		// which the state is reset, assuming that the remote clock has jumped.
		static constexpr int64_t ArrivalTimeOffsetThresholdUs{ 3000000 };

		/* Instance methods. */

		InterArrivalDelta::InterArrivalDelta(uint64_t sendTimeGroupLengthUs)
		  : sendTimeGroupLengthUs(sendTimeGroupLengthUs)
		{
			MS_TRACE();
		}

		std::optional<InterArrivalDelta::Deltas> InterArrivalDelta::ComputeDeltas(
		  uint64_t sendTimeUs, uint64_t arrivalTimeUs, uint64_t feedbackAtUs, size_t packetSize)
		{
			MS_TRACE();

			std::optional<Deltas> deltas;

			if (this->currentGroup.IsFirstPacket())
			{
				// Not enough data to compute deltas yet, so just store the packet until
				// there are two groups to compare.
				this->currentGroup.firstSendTimeUs    = sendTimeUs;
				this->currentGroup.sendTimeUs         = sendTimeUs;
				this->currentGroup.firstArrivalTimeUs = arrivalTimeUs;
			}
			else if (this->currentGroup.firstSendTimeUs > sendTimeUs)
			{
				// Reordered packet.
				return std::nullopt;
			}
			else if (IsNewSendTimeGroup(arrivalTimeUs, sendTimeUs))
			{
				// This is the first packet of a later send burst, so the sample of the
				// ongoing group is ready.
				if (!this->prevGroup.IsFirstPacket())
				{
					const int64_t sendDeltaUs = static_cast<int64_t>(this->currentGroup.sendTimeUs) -
					                            static_cast<int64_t>(this->prevGroup.sendTimeUs);
					const int64_t arrivalDeltaUs =
					  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					  static_cast<int64_t>(this->currentGroup.completeTimeUs.value()) -
					  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					  static_cast<int64_t>(this->prevGroup.completeTimeUs.value());
					const int64_t ownDeltaUs = static_cast<int64_t>(this->currentGroup.lastFeedbackAtUs) -
					                           static_cast<int64_t>(this->prevGroup.lastFeedbackAtUs);

					if (arrivalDeltaUs - ownDeltaUs >= ArrivalTimeOffsetThresholdUs)
					{
						MS_WARN_TAG(
						  bwe,
						  "arrival time clock offset has changed, resetting [diff:%" PRIi64 " us]",
						  arrivalDeltaUs - ownDeltaUs);

						Reset();

						return std::nullopt;
					}

					if (arrivalDeltaUs < 0)
					{
						// The groups have been reordered after their arrival time was taken.
						++this->numConsecutiveReorderedGroups;

						if (this->numConsecutiveReorderedGroups >= ReorderedResetThreshold)
						{
							MS_WARN_TAG(
							  bwe,
							  "packets between send bursts arrived out of order, resetting [arrivalDelta:%" PRIi64
							  " us, sendDelta:%" PRIi64 " us]",
							  arrivalDeltaUs,
							  sendDeltaUs);

							Reset();
						}

						return std::nullopt;
					}

					this->numConsecutiveReorderedGroups = 0;

					deltas = Deltas{ .sendDeltaUs    = sendDeltaUs,
						               .arrivalDeltaUs = arrivalDeltaUs,
						               .sizeDelta      = static_cast<int64_t>(this->currentGroup.size) -
						                                 static_cast<int64_t>(this->prevGroup.size) };
				}

				this->prevGroup = this->currentGroup;

				// The new packet starts the current group.
				this->currentGroup.firstSendTimeUs    = sendTimeUs;
				this->currentGroup.sendTimeUs         = sendTimeUs;
				this->currentGroup.firstArrivalTimeUs = arrivalTimeUs;
				this->currentGroup.size               = 0;
			}
			else
			{
				this->currentGroup.sendTimeUs = std::max(this->currentGroup.sendTimeUs, sendTimeUs);
			}

			// Accumulate the group size.
			this->currentGroup.size += packetSize;
			this->currentGroup.completeTimeUs   = arrivalTimeUs;
			this->currentGroup.lastFeedbackAtUs = feedbackAtUs;

			return deltas;
		}

		bool InterArrivalDelta::IsNewSendTimeGroup(uint64_t arrivalTimeUs, uint64_t sendTimeUs) const
		{
			MS_TRACE();

			if (this->currentGroup.IsFirstPacket())
			{
				return false;
			}
			else if (BelongsToBurst(arrivalTimeUs, sendTimeUs))
			{
				return false;
			}
			else
			{
				// NOTE: Safe unsigned subtraction since the caller already discarded
				// packets sent before the first one of the current group.
				return sendTimeUs - this->currentGroup.firstSendTimeUs > this->sendTimeGroupLengthUs;
			}
		}

		bool InterArrivalDelta::BelongsToBurst(uint64_t arrivalTimeUs, uint64_t sendTimeUs) const
		{
			MS_TRACE();

			MS_ASSERT(!this->currentGroup.IsFirstPacket(), "current group is empty");

			const int64_t arrivalDeltaUs = static_cast<int64_t>(arrivalTimeUs) -
			                               // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			                               static_cast<int64_t>(this->currentGroup.completeTimeUs.value());
			const int64_t sendDeltaUs =
			  static_cast<int64_t>(sendTimeUs) - static_cast<int64_t>(this->currentGroup.sendTimeUs);

			// Packets sent at the very same time always belong to the same burst.
			if (sendDeltaUs == 0)
			{
				return true;
			}

			const int64_t propagationDeltaUs = arrivalDeltaUs - sendDeltaUs;
			const int64_t burstDurationUs = static_cast<int64_t>(arrivalTimeUs) -
			                                static_cast<int64_t>(this->currentGroup.firstArrivalTimeUs);

			// The packet arrived earlier than its send time gap would suggest, meaning
			// that it was serialized by the network together with the ongoing group.
			return propagationDeltaUs < 0 && arrivalDeltaUs <= BurstDeltaThresholdUs &&
			       burstDurationUs < MaxBurstDurationUs;
		}

		void InterArrivalDelta::Reset()
		{
			MS_TRACE();

			this->numConsecutiveReorderedGroups = 0;
			this->currentGroup                  = SendTimeGroup();
			this->prevGroup                     = SendTimeGroup();
		}
	} // namespace BWE
} // namespace RTC
