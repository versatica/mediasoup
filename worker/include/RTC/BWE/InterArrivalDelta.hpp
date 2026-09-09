#ifndef MS_RTC_BWE_INTER_ARRIVAL_DELTA_HPP
#define MS_RTC_BWE_INTER_ARRIVAL_DELTA_HPP

#include "common.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Groups sent packets into send bursts and computes the deltas between two
		 * consecutive bursts.
		 *
		 * All packets sent within `sendTimeGroupLengthUs` of the first packet of a
		 * group belong to that group. This means that a burst of packets written
		 * into the socket at once produces a single sample rather than one per
		 * packet, which is what prevents a key frame being forwarded as a burst from
		 * being read as network congestion.
		 *
		 * The computed deltas are the input of the trendline estimator: if the
		 * arrival delta grows faster than the send delta then queues are building up
		 * somewhere in the network.
		 */
		class InterArrivalDelta
		{
		public:
			/**
			 * Deltas between two consecutive groups of packets sent in a burst.
			 */
			struct Deltas
			{
				/**
				 * Time elapsed between the send times of both groups (us).
				 */
				int64_t sendDeltaUs;

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
			struct SendTimeGroup
			{
				bool IsFirstPacket() const
				{
					return !this->completeTimeUs.has_value();
				}

				size_t size{ 0 };
				int64_t firstSendTimeUs{ 0 };
				int64_t sendTimeUs{ 0 };
				int64_t firstArrivalTimeUs{ 0 };
				std::optional<int64_t> completeTimeUs;
				int64_t lastFeedbackAtUs{ 0 };
			};

		public:
			/**
			 * @param sendTimeGroupLengthUs - Every packet sent within this time of the
			 *   first packet of a group belongs to that group.
			 */
			explicit InterArrivalDelta(int64_t sendTimeGroupLengthUs);

			/**
			 * Feed a sent packet whose arrival time is already known.
			 *
			 * @param sendTimeUs - Time at which the packet was sent.
			 * @param arrivalTimeUs - Time at which the packet arrived, in the remote
			 *   clock reference.
			 * @param feedbackAtUs - Time, in our own clock reference, at which the
			 *   RTCP feedback reporting this packet was received. It's the very same
			 *   value for every packet within a given feedback, and it's used to
			 *   detect that the remote clock has jumped by comparing how much it
			 *   advanced against how much ours did.
			 * @param packetSize - Size of the packet (bytes).
			 *
			 * @returns The deltas between the two latest complete groups, or no value
			 *   if they cannot be computed yet.
			 */
			std::optional<Deltas> ComputeDeltas(
			  int64_t sendTimeUs, int64_t arrivalTimeUs, int64_t feedbackAtUs, size_t packetSize);

		private:
			bool IsNewSendTimeGroup(int64_t arrivalTimeUs, int64_t sendTimeUs) const;

			bool BelongsToBurst(int64_t arrivalTimeUs, int64_t sendTimeUs) const;

			void Reset();

		private:
			const int64_t sendTimeGroupLengthUs;
			SendTimeGroup currentGroup;
			SendTimeGroup prevGroup;
			size_t numConsecutiveReorderedGroups{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
