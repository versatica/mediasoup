#ifndef MS_RTC_BWE_INTER_ARRIVAL_HPP
#define MS_RTC_BWE_INTER_ARRIVAL_HPP

#include "common.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Groups received packets into send bursts and computes the deltas between
		 * two consecutive bursts.
		 *
		 * It does the same job as `InterArrivalDelta` but on the receiving side,
		 * where the instant a packet was sent is not known: all there is is a
		 * timestamp of the sender's own, carried by the packet, which wraps around
		 * and whose tick is whatever the sender decided. So groups are formed by
		 * that timestamp instead of by a real instant, and telling which of two
		 * timestamps is the later one has to account for the wrap.
		 *
		 * The computed deltas are the input of the overuse estimator: if the arrival
		 * delta grows faster than the send delta then queues are building up
		 * somewhere in the network.
		 */
		class InterArrival
		{
		public:
			/**
			 * Deltas between two consecutive groups of packets sent in a burst.
			 */
			struct Deltas
			{
				/**
				 * Ticks elapsed between the send timestamps of both groups.
				 */
				uint32_t timestampDelta;

				/**
				 * Time elapsed between the arrival times of both groups (us).
				 */
				int64_t arrivalDeltaUs;

				/**
				 * Difference of size between both groups (bytes).
				 */
				int64_t sizeDelta;
			};

		private:
			struct TimestampGroup
			{
				bool IsFirstPacket() const
				{
					return !this->completeTimeUs.has_value();
				}

				size_t size{ 0 };
				uint32_t firstTimestamp{ 0 };
				uint32_t timestamp{ 0 };
				int64_t firstArrivalTimeUs{ 0 };
				std::optional<int64_t> completeTimeUs;
				int64_t lastSystemTimeUs{ 0 };
			};

		public:
			/**
			 * @param timestampGroupLengthTicks - Every packet whose timestamp is
			 *   within this many ticks of the first packet of a group belongs to that
			 *   group.
			 * @param timestampToUsCoeff - Microseconds a single tick of those
			 *   timestamps is worth.
			 */
			InterArrival(uint32_t timestampGroupLengthTicks, double timestampToUsCoeff);

			/**
			 * Feed a received packet.
			 *
			 * @param timestamp - Timestamp the packet carries, in the sender's own
			 *   units, which wraps around.
			 * @param arrivalTimeUs - Time at which the packet arrived.
			 * @param systemTimeUs - Time, in our own clock reference, at which the
			 *   packet is being fed. It's used to detect that the timestamps have
			 *   jumped by comparing how much they advanced against how much our clock
			 *   did.
			 * @param packetSize - Size of the packet (bytes).
			 *
			 * @returns The deltas between the two latest complete groups, or no value
			 *   if they cannot be computed yet.
			 */
			std::optional<Deltas> ComputeDeltas(
			  uint32_t timestamp, int64_t arrivalTimeUs, int64_t systemTimeUs, size_t packetSize);

		private:
			/**
			 * Whether the given timestamp is not older than the first one of the
			 * current group, so that a packet that got reordered on its way here
			 * doesn't close a group it doesn't belong to.
			 */
			bool IsPacketInOrder(uint32_t timestamp) const;

			bool IsNewTimestampGroup(int64_t arrivalTimeUs, uint32_t timestamp) const;

			bool BelongsToBurst(int64_t arrivalTimeUs, uint32_t timestamp) const;

			void Reset();

		private:
			const uint32_t timestampGroupLengthTicks;
			const double timestampToUsCoeff;
			TimestampGroup currentGroup;
			TimestampGroup prevGroup;
			size_t numConsecutiveReorderedGroups{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
