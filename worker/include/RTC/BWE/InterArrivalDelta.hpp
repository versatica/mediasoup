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
		 * All packets sent within `SendTimeGroupLengthMs` of the first packet of a
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
				 * Time elapsed between the send times of both groups (ms).
				 */
				int64_t sendDeltaMs;

				/**
				 * Time elapsed between the arrival times of both groups (ms).
				 */
				int64_t arrivalDeltaMs;

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
					return !this->completeTimeMs.has_value();
				}

				size_t size{ 0 };
				uint64_t firstSendTimeMs{ 0 };
				uint64_t sendTimeMs{ 0 };
				uint64_t firstArrivalTimeMs{ 0 };
				std::optional<uint64_t> completeTimeMs;
				uint64_t lastFeedbackAtMs{ 0 };
			};

		public:
			/**
			 * Feed a sent packet whose arrival time is already known.
			 *
			 * @param sendTimeMs - Time at which the packet was sent.
			 * @param arrivalTimeMs - Time at which the packet arrived, in the remote
			 *   clock reference.
			 * @param feedbackAtMs - Time, in our own clock reference, at which the
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
			  uint64_t sendTimeMs, uint64_t arrivalTimeMs, uint64_t feedbackAtMs, size_t packetSize);

		private:
			bool IsNewSendTimeGroup(uint64_t arrivalTimeMs, uint64_t sendTimeMs) const;

			bool BelongsToBurst(uint64_t arrivalTimeMs, uint64_t sendTimeMs) const;

			void Reset();

		private:
			SendTimeGroup currentGroup;
			SendTimeGroup prevGroup;
			size_t numConsecutiveReorderedGroups{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
