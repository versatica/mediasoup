#define MS_CLASS "RTC::BWE::BitrateAllocator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/BitrateAllocator.hpp"
#include "Logger.hpp"
#include "RTC/BWE/BitrateUtils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// What a point above the one a candidate is running at costs on top of its
		// own price, which is what keeps a link hovering around that price from
		// having the candidate go up and down.
		static constexpr double HysteresisFactor{ 1.2 };

		/* Instance methods. */

		void BitrateAllocator::Distribute(int64_t availableBitrate, const std::vector<Candidate*>& candidates)
		{
			MS_TRACE();

			// Whoever wants bitrate, heaviest first and, within the same weight, by id,
			// so that the outcome of a round does not depend on where each one happens
			// to sit in the map of the caller.
			std::vector<Entry> entries;

			entries.reserve(candidates.size());

			int64_t totalWeight{ 0 };

			for (auto* candidate : candidates)
			{
				auto bitrateOptions = candidate->GetBitrateOptions();

				MS_ASSERT(!bitrateOptions.options.empty(), "candidate offers no point at all");

				// Everyone takes at least its cheapest point, which is the least it may
				// send and so is not up for discussion. For one that can move that is
				// the point of sending nothing, priced at zero, while one that cannot
				// takes what it is putting out off the top.
				availableBitrate -= bitrateOptions.options.front().bitrate;

				// A candidate with a single point has nothing to bid for, so it does
				// not take part in the round beyond the price just subtracted. That is
				// a simple sender, of audio or of video, and a pipe one, since both
				// forward whole streams and have no layers to pick. And also one that
				// is paused or inactive for any other reason, whose one point is
				// priced at zero.
				if (bitrateOptions.options.size() == 1)
				{
					continue;
				}

				const uint8_t weight = candidate->GetBitrateWeight();

				if (weight > 0)
				{
					entries.push_back(
					  Entry{ .candidate     = candidate,
						       .options       = std::move(bitrateOptions.options),
						       .currentOption = bitrateOptions.currentOption });

					totalWeight += weight;
				}
			}

			// What cannot be moved may already be more than the downlink bears.
			availableBitrate = std::max<int64_t>(0, availableBitrate);

			if (entries.empty())
			{
				return;
			}

			std::ranges::sort(
			  entries,
			  [](const Entry& a, const Entry& b) -> bool
			  {
				  if (a.candidate->GetBitrateWeight() != b.candidate->GetBitrateWeight())
				  {
					  return a.candidate->GetBitrateWeight() > b.candidate->GetBitrateWeight();
				  }
				  else
				  {
					  return a.candidate->GetBitrateAllocationId() < b.candidate->GetBitrateAllocationId();
				  }
			  });

			RotateWeightGroups(entries);

			// First round: one point each, in order, so that every candidate climbs
			// its first before anybody climbs a second. This is the floor every
			// candidate is entitled to, and giving more than one point here would
			// hand whoever goes first everything there is.
			for (auto& entry : entries)
			{
				const int64_t climbBitrate = GetClimbBitrate(entry);

				if (climbBitrate <= availableBitrate)
				{
					entry.option++;

					availableBitrate -= climbBitrate;
				}
			}

			// Later rounds: what is left is split by weight, so that twice the weight
			// is twice the share. What a candidate does not take stays for the next
			// round, since a share may be too small for any point of its own.
			while (availableBitrate > 0)
			{
				const int64_t bitrateBeforeRound = availableBitrate;

				for (auto& entry : entries)
				{
					// NOTE: Taken out of what the round started with and not out of what
					// is left right now, since otherwise the share of each one would be
					// smaller than the share of whoever went before it and the order
					// would decide the outcome. There is always enough left to cover it:
					// the shares are whole numbers, so they add up to no more than what
					// the round started with, and nobody takes more than its own.
					const int64_t share =
					  (bitrateBeforeRound * entry.candidate->GetBitrateWeight()) / totalWeight;

					availableBitrate -= ClimbWith(entry, share);
				}

				if (availableBitrate == bitrateBeforeRound)
				{
					// Nobody could climb with its share. What is in the way may be the
					// share rather than the bitrate, since a share is a fraction of what
					// is left and the price of a point is not, so what is left is offered
					// whole before giving up. Otherwise bitrate would sit unused while
					// someone could have taken it.
					for (auto& entry : entries)
					{
						availableBitrate -= ClimbWith(entry, availableBitrate);
					}

					// Not even that moved anything, so no further round will.
					if (availableBitrate == bitrateBeforeRound)
					{
						break;
					}
				}
			}

			// Nothing is committed until the whole round is over, since a candidate may
			// climb more than one point and applying each climb on its own would switch
			// layers on the way to the same place.
			for (const auto& entry : entries)
			{
				if (!entry.currentOption.has_value() || entry.currentOption.value() != entry.option)
				{
					entry.candidate->SetBitrateOption(entry.options.at(entry.option));
				}
			}

			this->numDistributions++;
		}

		int64_t BitrateAllocator::ClimbWith(Entry& entry, int64_t bitrate)
		{
			MS_TRACE();

			int64_t takenBitrate{ 0 };

			// A candidate may climb more than one point with what it is offered, and
			// may climb one that costs nothing or gives bitrate back.
			while (entry.option + 1 < entry.options.size())
			{
				const int64_t climbBitrate = GetClimbBitrate(entry);

				if (climbBitrate > bitrate - takenBitrate)
				{
					break;
				}

				takenBitrate += climbBitrate;

				entry.option++;
			}

			return takenBitrate;
		}

		int64_t BitrateAllocator::GetClimbBitrate(const Entry& entry)
		{
			MS_TRACE();

			MS_ASSERT(entry.option + 1 < entry.options.size(), "no point above the one being held");

			const int64_t heldBitrate = entry.options.at(entry.option).bitrate;
			const int64_t nextBitrate = entry.options.at(entry.option + 1).bitrate;

			// A candidate that has never been given a point is not climbing back to
			// anything, so it pays the price and no more.
			const bool isClimbingBack =
			  entry.currentOption.has_value() && entry.option + 1 > entry.currentOption.value();

			const int64_t requiredBitrate =
			  isClimbingBack ? BitrateUtils::ApplyBitrateFactor(nextBitrate, HysteresisFactor)
				               : nextBitrate;

			return requiredBitrate - heldBitrate;
		}

		void BitrateAllocator::RotateWeightGroups(std::vector<Entry>& entries) const
		{
			MS_TRACE();

			// Within a group of equal weight the order is otherwise always the same,
			// so the first one would always be the one to get the point that nothing
			// left over could pay for. Each round starts the group one position
			// further along.
			auto groupBegin = entries.begin();

			while (groupBegin != entries.end())
			{
				const uint8_t weight = groupBegin->candidate->GetBitrateWeight();

				auto groupEnd = groupBegin;

				while (groupEnd != entries.end() && groupEnd->candidate->GetBitrateWeight() == weight)
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
