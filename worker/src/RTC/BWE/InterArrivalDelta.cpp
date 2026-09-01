#define MS_CLASS "RTC::BWE::InterArrivalDelta"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/InterArrivalDelta.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// A send time group holds every packet sent within this time of the first
		// packet of the group.
		static constexpr uint64_t SendTimeGroupLengthMs{ 5 };
		// Maximum arrival time delta for a packet to be absorbed into the ongoing
		// burst.
		static constexpr int64_t BurstDeltaThresholdMs{ 5 };
		// Maximum duration of a burst.
		static constexpr int64_t MaxBurstDurationMs{ 100 };
		// Number of consecutive reordered groups after which the state is reset.
		static constexpr size_t ReorderedResetThreshold{ 3 };
		// Difference between the arrival time delta and our own time delta after
		// which the state is reset, assuming that the remote clock has jumped.
		static constexpr int64_t ArrivalTimeOffsetThresholdMs{ 3000 };

		/* Instance methods. */

		std::optional<InterArrivalDelta::Deltas> InterArrivalDelta::ComputeDeltas(
		  uint64_t sendTimeMs, uint64_t arrivalTimeMs, uint64_t feedbackAtMs, size_t packetSize)
		{
			MS_TRACE();

			std::optional<Deltas> deltas;

			if (this->currentGroup.IsFirstPacket())
			{
				// Not enough data to compute deltas yet, so just store the packet until
				// there are two groups to compare.
				this->currentGroup.firstSendTimeMs    = sendTimeMs;
				this->currentGroup.sendTimeMs         = sendTimeMs;
				this->currentGroup.firstArrivalTimeMs = arrivalTimeMs;
			}
			else if (this->currentGroup.firstSendTimeMs > sendTimeMs)
			{
				// Reordered packet.
				return std::nullopt;
			}
			else if (IsNewSendTimeGroup(arrivalTimeMs, sendTimeMs))
			{
				// This is the first packet of a later send burst, so the sample of the
				// ongoing group is ready.
				if (!this->prevGroup.IsFirstPacket())
				{
					const int64_t sendDeltaMs = static_cast<int64_t>(this->currentGroup.sendTimeMs) -
					                            static_cast<int64_t>(this->prevGroup.sendTimeMs);
					const int64_t arrivalDeltaMs =
					  static_cast<int64_t>(this->currentGroup.completeTimeMs.value()) -
					  static_cast<int64_t>(this->prevGroup.completeTimeMs.value());
					const int64_t ownDeltaMs = static_cast<int64_t>(this->currentGroup.lastFeedbackAtMs) -
					                           static_cast<int64_t>(this->prevGroup.lastFeedbackAtMs);

					if (arrivalDeltaMs - ownDeltaMs >= ArrivalTimeOffsetThresholdMs)
					{
						MS_WARN_TAG(
						  bwe,
						  "arrival time clock offset has changed, resetting [diff:%" PRIi64 " ms]",
						  arrivalDeltaMs - ownDeltaMs);

						Reset();

						return std::nullopt;
					}

					if (arrivalDeltaMs < 0)
					{
						// The groups have been reordered after their arrival time was taken.
						++this->numConsecutiveReorderedGroups;

						if (this->numConsecutiveReorderedGroups >= ReorderedResetThreshold)
						{
							MS_WARN_TAG(
							  bwe,
							  "packets between send bursts arrived out of order, resetting [arrivalDelta:%" PRIi64
							  " ms, sendDelta:%" PRIi64 " ms]",
							  arrivalDeltaMs,
							  sendDeltaMs);

							Reset();
						}

						return std::nullopt;
					}

					this->numConsecutiveReorderedGroups = 0;

					deltas = Deltas{ .sendDeltaMs    = sendDeltaMs,
						               .arrivalDeltaMs = arrivalDeltaMs,
						               .sizeDelta      = static_cast<int64_t>(this->currentGroup.size) -
						                                 static_cast<int64_t>(this->prevGroup.size) };
				}

				this->prevGroup = this->currentGroup;

				// The new packet starts the current group.
				this->currentGroup.firstSendTimeMs    = sendTimeMs;
				this->currentGroup.sendTimeMs         = sendTimeMs;
				this->currentGroup.firstArrivalTimeMs = arrivalTimeMs;
				this->currentGroup.size               = 0;
			}
			else
			{
				this->currentGroup.sendTimeMs = std::max(this->currentGroup.sendTimeMs, sendTimeMs);
			}

			// Accumulate the group size.
			this->currentGroup.size += packetSize;
			this->currentGroup.completeTimeMs   = arrivalTimeMs;
			this->currentGroup.lastFeedbackAtMs = feedbackAtMs;

			return deltas;
		}

		bool InterArrivalDelta::IsNewSendTimeGroup(uint64_t arrivalTimeMs, uint64_t sendTimeMs) const
		{
			MS_TRACE();

			if (this->currentGroup.IsFirstPacket())
			{
				return false;
			}
			else if (BelongsToBurst(arrivalTimeMs, sendTimeMs))
			{
				return false;
			}
			else
			{
				// NOTE: Safe unsigned subtraction since the caller already discarded
				// packets sent before the first one of the current group.
				return sendTimeMs - this->currentGroup.firstSendTimeMs > SendTimeGroupLengthMs;
			}
		}

		bool InterArrivalDelta::BelongsToBurst(uint64_t arrivalTimeMs, uint64_t sendTimeMs) const
		{
			MS_TRACE();

			MS_ASSERT(!this->currentGroup.IsFirstPacket(), "current group is empty");

			const int64_t arrivalDeltaMs = static_cast<int64_t>(arrivalTimeMs) -
			                               static_cast<int64_t>(this->currentGroup.completeTimeMs.value());
			const int64_t sendDeltaMs =
			  static_cast<int64_t>(sendTimeMs) - static_cast<int64_t>(this->currentGroup.sendTimeMs);

			// Packets sent at the very same time always belong to the same burst.
			if (sendDeltaMs == 0)
			{
				return true;
			}

			const int64_t propagationDeltaMs = arrivalDeltaMs - sendDeltaMs;
			const int64_t burstDurationMs = static_cast<int64_t>(arrivalTimeMs) -
			                                static_cast<int64_t>(this->currentGroup.firstArrivalTimeMs);

			// The packet arrived earlier than its send time gap would suggest, meaning
			// that it was serialized by the network together with the ongoing group.
			return propagationDeltaMs < 0 && arrivalDeltaMs <= BurstDeltaThresholdMs &&
			       burstDurationMs < MaxBurstDurationMs;
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
