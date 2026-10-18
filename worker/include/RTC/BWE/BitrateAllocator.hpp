#ifndef MS_RTC_BWE_BITRATE_ALLOCATOR_HPP
#define MS_RTC_BWE_BITRATE_ALLOCATOR_HPP

#include "common.hpp"
#include "RTC/ConsumerTypes.hpp"
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
		 * - What a candidate may send is not a continuous range but a handful of
		 *   operating points, since it forwards what somebody else encoded: one
		 *   combination of layers, at whatever that combination costs. So a
		 *   candidate offers the points it may run at with their price and this
		 *   picks one for each.
		 * - Not every candidate has any choice: audio, pipe and a sender of a
		 *   single stream offer the single point they are running at, which is
		 *   taken off the top and never bid for, since it takes up the downlink
		 *   just the same.
		 * - Climbing to a point above the one a candidate is running at costs more
		 *   than the point is worth, so that a link hovering around a price does
		 *   not have it going up and down. Dropping to a lower one costs just its
		 *   price.
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
				/**
				 * One of the points a candidate may run at.
				 */
				struct BitrateOption
				{
					/**
					 * What running at it costs (bps).
					 */
					int64_t bitrate;
					/**
					 * Layers the candidate runs at in it, both -1 for none at all.
					 *
					 * @remarks
					 * - Never looked at here, just handed back to the candidate when
					 *   this is the point it is given. A candidate with a single point
					 *   leaves it alone, since it is never given any point back.
					 */
					RTC::ConsumerTypes::VideoLayers layers;
				};

				/**
				 * The points a candidate may run at and which of them it is running
				 * at now.
				 */
				struct BitrateOptions
				{
					/**
					 * The points, least preferred first, which is normally cheapest
					 * first too. Never empty: a candidate with nothing to choose offers
					 * the single point it is running at.
					 *
					 * @remarks
					 * - Not sending anything is one of them, priced at zero, whenever
					 *   the candidate may be left silent.
					 */
					std::vector<BitrateOption> options;
					/**
					 * Which of them it is running at, or no value while it has never
					 * been given one.
					 *
					 * @remarks
					 * - A candidate that was dropped to sending nothing is running at
					 *   that point rather than at none, which is what keeps it from
					 *   climbing back without the margin a fresh one does not pay.
					 */
					std::optional<size_t> currentOption;
				};

				virtual ~Candidate() = default;

			public:
				/**
				 * Tells one candidate from another, so that a round is reproducible
				 * rather than ordered by wherever they happen to sit in a map.
				 */
				virtual const std::string& GetBitrateAllocationId() const = 0;

				/**
				 * How much of what is left over this one gets relative to the others.
				 * Twice the weight is twice the share of what the first round did not
				 * take.
				 *
				 * @returns Zero if it wants nothing, in which case it is left out of
				 *   the round.
				 */
				virtual uint8_t GetBitrateWeight() const = 0;

				/**
				 * The points this one may run at right now, which change as the layers
				 * of what it forwards come and go.
				 *
				 * @remarks
				 * - Not sending at all is a point like any other, priced at zero,
				 *   which is what tells a candidate that was dropped to nothing from
				 *   one that has never been given anything.
				 */
				virtual BitrateOptions GetBitrateOptions() const = 0;

				/**
				 * Run at the given point from now on.
				 *
				 * @remarks
				 * - Only called once the whole round is over, only when the point is
				 *   not the one it was already running at, and never on a candidate
				 *   that offered a single point.
				 */
				virtual void SetBitrateOption(const BitrateOption& option) = 0;
			};

		public:
			/**
			 * Give `availableBitrate` out among the candidates that want it.
			 *
			 * @remarks
			 * - The candidates are given on each call rather than kept, so that a
			 *   candidate that goes away needs no unregistering anywhere.
			 * - Every candidate is given, with points to choose from or without.
			 *   What the ones without offer is taken off `availableBitrate` first,
			 *   so the caller has nothing to work out beforehand.
			 * - No candidate may be destroyed while this runs, since the rest are
			 *   held as plain pointers across the calls made into each one.
			 */
			void Distribute(int64_t availableBitrate, const std::vector<Candidate*>& candidates);

		private:
			/**
			 * A candidate taking part in a round, along with the points it offered
			 * and the one it has been given so far.
			 */
			struct Entry
			{
				Candidate* candidate{ nullptr };
				std::vector<Candidate::BitrateOption> options;
				std::optional<size_t> currentOption;
				size_t option{ 0 };
			};

		private:
			/**
			 * Climb a candidate as many points as the given bitrate pays for and
			 * tell how much of it was spent.
			 */
			static int64_t ClimbWith(Entry& entry, int64_t bitrate);

			/**
			 * What a candidate has to be given to climb from the point it holds to
			 * the one above it, including the margin that keeps it from climbing
			 * back the instant it is dropped.
			 */
			static int64_t GetClimbBitrate(const Entry& entry);

			/**
			 * Move the starting point of every group of equal weight one position
			 * further along than the previous round did.
			 */
			void RotateWeightGroups(std::vector<Entry>& entries) const;

		private:
			// Rounds run so far, which is what moves the starting point of every group
			// of equal weight so that the point nobody else could pay for does not
			// always fall to the same candidate.
			uint64_t numDistributions{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
