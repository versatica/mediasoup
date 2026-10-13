#define MS_CLASS "RTC::BWE::BitrateAllocator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/BitrateAllocator.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		void BitrateAllocator::Distribute(int64_t availableBitrate, const std::vector<Candidate*>& candidates)
		{
			MS_TRACE();

			// Whoever wants bitrate, heaviest first and, within the same weight, by id,
			// so that the outcome of a round does not depend on where each one happens
			// to sit in the map of the caller.
			std::vector<Candidate*> wantingCandidates;

			wantingCandidates.reserve(candidates.size());

			for (auto* candidate : candidates)
			{
				// What a candidate that cannot change its bitrate puts out is not up
				// for grabs, so it comes off the top instead of being bid for.
				if (!candidate->IsBitrateAdaptable())
				{
					availableBitrate -= candidate->GetOutputBitrate();

					continue;
				}

				if (candidate->GetBitrateWeight() > 0)
				{
					wantingCandidates.push_back(candidate);
				}
			}

			// What cannot be moved may already be more than the downlink bears.
			availableBitrate = std::max<int64_t>(0, availableBitrate);

			if (wantingCandidates.empty())
			{
				return;
			}

			std::ranges::sort(
			  wantingCandidates,
			  [](const Candidate* a, const Candidate* b) -> bool
			  {
				  if (a->GetBitrateWeight() != b->GetBitrateWeight())
				  {
					  return a->GetBitrateWeight() > b->GetBitrateWeight();
				  }
				  else
				  {
					  return a->GetBitrateAllocationId() < b->GetBitrateAllocationId();
				  }
			  });

			RotateWeightGroups(wantingCandidates);

			// First round: everyone is offered everything that is left, in order, so
			// that each one gets to climb its first step before anybody climbs a
			// second. This is the floor every candidate is entitled to.
			for (auto* candidate : wantingCandidates)
			{
				availableBitrate -= TakeFrom(candidate, availableBitrate);
			}

			// Later rounds: what is left is split by weight, so that twice the weight
			// is twice the share. What a candidate does not take stays for the next
			// round, since a share may be too small for any step of its own.
			while (availableBitrate > 0)
			{
				const int64_t bitrateBeforeRound = availableBitrate;

				int64_t totalWeight{ 0 };

				for (const auto* candidate : wantingCandidates)
				{
					totalWeight += candidate->GetBitrateWeight();
				}

				for (auto* candidate : wantingCandidates)
				{
					// NOTE: Taken out of what the round started with and not out of what
					// is left right now, since otherwise the share of each one would be
					// smaller than the share of whoever went before it and the order
					// would decide the outcome. There is always enough left to cover it:
					// the shares are whole numbers, so they add up to no more than what
					// the round started with, and nobody takes more than its own.
					const int64_t share = (bitrateBeforeRound * candidate->GetBitrateWeight()) / totalWeight;

					availableBitrate -= TakeFrom(candidate, share);
				}

				if (availableBitrate == bitrateBeforeRound)
				{
					// Nobody could climb a step with its share. What is in the way may be
					// the share rather than the bitrate, since a share is a fraction of
					// what is left and a step is not, so what is left is offered whole
					// before giving up. Otherwise bitrate would sit unused while someone
					// could have taken it.
					for (auto* candidate : wantingCandidates)
					{
						availableBitrate -= TakeFrom(candidate, availableBitrate);
					}

					// Not even that moved anything, so no further round will.
					if (availableBitrate == bitrateBeforeRound)
					{
						break;
					}
				}
			}

			for (auto* candidate : wantingCandidates)
			{
				candidate->ApplyBitrateAllocation();
			}

			this->numDistributions++;
		}

		int64_t BitrateAllocator::TakeFrom(Candidate* candidate, int64_t bitrate)
		{
			MS_TRACE();

			if (bitrate <= 0)
			{
				return 0;
			}

			const int64_t takenBitrate = candidate->TakeBitrate(bitrate);

			MS_ASSERT(
			  takenBitrate <= bitrate,
			  "candidate took more than it was offered [taken:%" PRIi64 ", offered:%" PRIi64 "]",
			  takenBitrate,
			  bitrate);

			return takenBitrate;
		}

		void BitrateAllocator::RotateWeightGroups(std::vector<Candidate*>& candidates) const
		{
			MS_TRACE();

			// Within a group of equal weight the order is otherwise always the same,
			// so the first one would always be the one to get the step that nothing
			// left over could pay for. Each round starts the group one position
			// further along.
			auto groupBegin = candidates.begin();

			while (groupBegin != candidates.end())
			{
				const uint8_t weight = (*groupBegin)->GetBitrateWeight();

				auto groupEnd = groupBegin;

				while (groupEnd != candidates.end() && (*groupEnd)->GetBitrateWeight() == weight)
				{
					++groupEnd;
				}

				const auto groupSize = static_cast<uint64_t>(std::distance(groupBegin, groupEnd));

				std::rotate(
				  groupBegin, groupBegin + static_cast<int64_t>(this->numDistributions % groupSize), groupEnd);

				groupBegin = groupEnd;
			}
		}
	} // namespace BWE
} // namespace RTC
