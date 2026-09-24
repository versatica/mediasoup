#define MS_CLASS "RTC::BWE::InterArrival"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/InterArrival.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cmath> // std::llround()

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

		InterArrival::InterArrival(uint32_t timestampGroupLengthTicks, double timestampToUsCoeff)
		  : timestampGroupLengthTicks(timestampGroupLengthTicks), timestampToUsCoeff(timestampToUsCoeff)
		{
			MS_TRACE();
		}

		std::optional<InterArrival::Deltas> InterArrival::ComputeDeltas(
		  uint32_t timestamp, int64_t arrivalTimeUs, int64_t systemTimeUs, size_t packetSize)
		{
			MS_TRACE();

			std::optional<Deltas> deltas;

			if (this->currentGroup.IsFirstPacket())
			{
				// Not enough data to compute deltas yet, so just store the packet until
				// there are two groups to compare.
				this->currentGroup.timestamp          = timestamp;
				this->currentGroup.firstTimestamp     = timestamp;
				this->currentGroup.firstArrivalTimeUs = arrivalTimeUs;
			}
			else if (!IsPacketInOrder(timestamp))
			{
				// Reordered packet.
				return std::nullopt;
			}
			else if (IsNewTimestampGroup(arrivalTimeUs, timestamp))
			{
				// This is the first packet of a later send burst, so the sample of the
				// ongoing group is ready.
				if (!this->prevGroup.IsFirstPacket())
				{
					const uint32_t timestampDelta = this->currentGroup.timestamp - this->prevGroup.timestamp;
					const int64_t arrivalDeltaUs =
					  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					  this->currentGroup.completeTimeUs.value() -
					  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					  this->prevGroup.completeTimeUs.value();
					const int64_t ownDeltaUs =
					  this->currentGroup.lastSystemTimeUs - this->prevGroup.lastSystemTimeUs;

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
							  " us]",
							  arrivalDeltaUs);

							Reset();
						}

						return std::nullopt;
					}

					this->numConsecutiveReorderedGroups = 0;

					deltas = Deltas{ .timestampDelta = timestampDelta,
					                 .arrivalDeltaUs = arrivalDeltaUs,
					                 .sizeDelta      = static_cast<int64_t>(this->currentGroup.size) -
						                                 static_cast<int64_t>(this->prevGroup.size) };
				}

				this->prevGroup = this->currentGroup;

				// The new packet starts the current group.
				this->currentGroup.firstTimestamp     = timestamp;
				this->currentGroup.timestamp          = timestamp;
				this->currentGroup.firstArrivalTimeUs = arrivalTimeUs;
				this->currentGroup.size               = 0;
			}
			else
			{
				// NOTE: Not std::max() because these timestamps wrap around, so the
				// later one is not necessarily the greater one.
				this->currentGroup.timestamp =
				  ::Utils::Number::IsHigherThan<uint32_t>(timestamp, this->currentGroup.timestamp)
				    ? timestamp
				    : this->currentGroup.timestamp;
			}

			// Accumulate the group size.
			this->currentGroup.size += packetSize;
			this->currentGroup.completeTimeUs   = arrivalTimeUs;
			this->currentGroup.lastSystemTimeUs = systemTimeUs;

			return deltas;
		}

		bool InterArrival::IsPacketInOrder(uint32_t timestamp) const
		{
			MS_TRACE();

			if (this->currentGroup.IsFirstPacket())
			{
				return true;
			}

			// A packet of the very same timestamp as the first one of the group is in
			// order, so this is not a strict comparison.
			return ::Utils::Number::IsHigherOrEqualThan<uint32_t>(
			  timestamp, this->currentGroup.firstTimestamp);
		}

		bool InterArrival::IsNewTimestampGroup(int64_t arrivalTimeUs, uint32_t timestamp) const
		{
			MS_TRACE();

			if (this->currentGroup.IsFirstPacket())
			{
				return false;
			}
			else if (BelongsToBurst(arrivalTimeUs, timestamp))
			{
				return false;
			}
			else
			{
				// NOTE: The subtraction is done in the width of the timestamps, so a
				// group that spans their wrap around still measures its own length.
				const uint32_t timestampDiff = timestamp - this->currentGroup.firstTimestamp;

				return timestampDiff > this->timestampGroupLengthTicks;
			}
		}

		bool InterArrival::BelongsToBurst(int64_t arrivalTimeUs, uint32_t timestamp) const
		{
			MS_TRACE();

			MS_ASSERT(!this->currentGroup.IsFirstPacket(), "current group is empty");

			const int64_t arrivalDeltaUs =
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  arrivalTimeUs - this->currentGroup.completeTimeUs.value();
			const uint32_t timestampDiff = timestamp - this->currentGroup.timestamp;
			// NOTE: Rounded rather than truncated, since a tick is worth a fraction of
			// a microsecond and truncating would call every short gap a zero.
			const auto sendDeltaUs = std::llround(this->timestampToUsCoeff * timestampDiff);

			// Packets sent at the very same time always belong to the same burst.
			if (sendDeltaUs == 0)
			{
				return true;
			}

			const int64_t propagationDeltaUs = arrivalDeltaUs - sendDeltaUs;
			const int64_t burstDurationUs    = arrivalTimeUs - this->currentGroup.firstArrivalTimeUs;

			// The packet arrived earlier than its send time gap would suggest, meaning
			// that it was serialized by the network together with the ongoing group.
			return propagationDeltaUs < 0 && arrivalDeltaUs <= BurstDeltaThresholdUs &&
			       burstDurationUs < MaxBurstDurationUs;
		}

		void InterArrival::Reset()
		{
			MS_TRACE();

			this->numConsecutiveReorderedGroups = 0;
			this->currentGroup                  = TimestampGroup();
			this->prevGroup                     = TimestampGroup();
		}
	} // namespace BWE
} // namespace RTC
