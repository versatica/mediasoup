#ifndef MS_RTC_BWE_BITRATE_ALLOCATOR_HPP
#define MS_RTC_BWE_BITRATE_ALLOCATOR_HPP

#include "common.hpp"
#include <string>
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Shares the available downlink bitrate out among the candidates that can
		 * use it.
		 *
		 * @remarks
		 * - Not every candidate can be given anything: audio, pipe and a sender of
		 *   a single stream have nothing to move, and what they are putting out is
		 *   taken off the top instead, since it takes up the downlink just the same.
		 * - What can be given out is not a continuous range but a set of steps: a
		 *   candidate only runs at the bitrate of one of the layer combinations it
		 *   may send, that set changes as the layers of what it forwards come and
		 *   go, and the application caps it on top. So the candidate is the one that
		 *   prices a step and decides whether it can take it, and this only holds
		 *   the purse.
		 * - Nothing is committed until the whole round is over, since a candidate may
		 *   climb more than one step and applying each climb on its own would switch
		 *   layers on the way to the same place.
		 */
		class BitrateAllocator
		{
		public:
			/**
			 * Whoever may be given bitrate.
			 */
			class Candidate
			{
			public:
				virtual ~Candidate() = default;

			public:
				/**
				 * Tells one candidate from another, so that a round is reproducible
				 * rather than ordered by wherever they happen to sit in a map.
				 */
				virtual const std::string& GetBitrateAllocationId() const = 0;

				/**
				 * Whether it can change the bitrate it puts out, which is what lets it
				 * take part in the round at all.
				 *
				 * @remarks
				 * - One that cannot still takes up the downlink, so what it puts out
				 *   comes off the top before anything is given out.
				 */
				virtual bool IsBitrateAdaptable() const = 0;

				/**
				 * What it is putting out right now.
				 *
				 * @remarks
				 * - Only asked of a candidate that is not adaptable, so an adaptable
				 *   one may assert instead of answering.
				 */
				virtual int64_t GetOutputBitrate() const = 0;

				/**
				 * How much of what is left over this one gets relative to the others.
				 * Twice the weight is twice the share of what the first round did not
				 * take.
				 *
				 * @returns Zero if it wants nothing, in which case it is left out of
				 *   the round.
				 *
				 * @remarks
				 * - Only asked of an adaptable candidate, so one that is not may
				 *   assert instead of answering.
				 */
				virtual uint8_t GetBitrateWeight() const = 0;

				/**
				 * Take up to the given bitrate, which means climbing one step if one
				 * fits in it.
				 *
				 * @returns What was taken, never more than what was offered, and zero
				 *   if nothing fits or there is nothing above where it already is.
				 *
				 * @remarks
				 * - What it takes is provisional until `ApplyBitrateAllocation()`.
				 * - Only offered to an adaptable candidate, so one that is not may
				 *   assert instead of answering.
				 */
				virtual int64_t TakeBitrate(int64_t bitrate) = 0;

				/**
				 * Commit what was taken over the whole round.
				 *
				 * @remarks
				 * - Only called on an adaptable candidate, so one that is not may
				 *   assert instead of doing anything.
				 */
				virtual void ApplyBitrateAllocation() = 0;
			};

		public:
			/**
			 * Give `availableBitrate` out among the candidates that want it.
			 *
			 * @remarks
			 * - The candidates are given on each call rather than kept, so that a
			 *   candidate that goes away needs no unregistering anywhere.
			 * - Every candidate is given, adaptable or not. What the ones that cannot
			 *   adapt are putting out is taken off `availableBitrate` first, so the
			 *   caller has nothing to work out beforehand.
			 * - No candidate may be destroyed while this runs, since the rest are
			 *   held as plain pointers across the calls made into each one.
			 */
			void Distribute(int64_t availableBitrate, const std::vector<Candidate*>& candidates);

		private:
			/**
			 * Offer a candidate up to the given bitrate and tell how much it took.
			 */
			static int64_t TakeFrom(Candidate* candidate, int64_t bitrate);

			/**
			 * Move the starting point of every group of equal weight one position
			 * further along than the previous round did.
			 */
			void RotateWeightGroups(std::vector<Candidate*>& candidates) const;

		private:
			// Rounds run so far, which is what moves the starting point of every group
			// of equal weight so that the step nobody else could pay for does not
			// always fall to the same candidate.
			uint64_t numDistributions{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
