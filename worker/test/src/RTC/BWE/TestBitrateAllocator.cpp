#include "common.hpp"
#include "RTC/BWE/BitrateAllocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

SCENARIO("BWE BitrateAllocator", "[bwe][bitrateallocator]")
{
	// A candidate that can run at any of the given bitrates and nowhere in
	// between, which is what a Consumer with its layers is.
	class TestCandidate : public RTC::BWE::BitrateAllocator::Candidate
	{
	public:
		TestCandidate(std::string id, uint8_t weight, std::vector<int64_t> bitrates)
		  : id(std::move(id)), weight(weight), bitrates(std::move(bitrates))
		{
		}

	public:
		const std::string& GetBitrateAllocationId() const override
		{
			return this->id;
		}

		uint8_t GetBitrateWeight() const override
		{
			return this->weight;
		}

		BitrateOptions GetBitrateOptions() const override
		{
			BitrateOptions bitrateOptions;

			// Running at nothing is one of the points, priced at zero.
			bitrateOptions.options.push_back({ .bitrate = 0, .layers = {} });

			for (size_t idx{ 0 }; idx <= this->maxPoint; ++idx)
			{
				bitrateOptions.options.push_back(
				  {
				    .bitrate = this->bitrates.at(idx), .layers = { 0, static_cast<int16_t>(idx) }
        });
			}

			bitrateOptions.currentOption = this->currentOption;

			return bitrateOptions;
		}

		void SetBitrateOption(const BitrateOption& option) override
		{
			this->bitrate = option.bitrate;

			this->timesGivenAPoint++;
		}

		// Bitrate it ended the round at.
		int64_t GetBitrate() const
		{
			return this->bitrate;
		}

		// How often it was told to run at a point, so that a round that walks it
		// through several of them is told from one that applies each one on the way.
		uint8_t GetTimesGivenAPoint() const
		{
			return this->timesGivenAPoint;
		}

		// Cap the application put on it, as an index into the bitrates.
		void SetMaxPoint(size_t maxPoint)
		{
			this->maxPoint = maxPoint;
		}

		// Where a previous round left it, as an index into the points it offers,
		// zero being not sending anything.
		void SetCurrentOption(size_t currentOption)
		{
			this->currentOption = currentOption;
			this->bitrate       = currentOption == 0 ? 0 : this->bitrates.at(currentOption - 1);
		}

	private:
		const std::string id;
		const uint8_t weight;
		const std::vector<int64_t> bitrates;
		size_t maxPoint{ 0 };

		// What it is running at, and which of its points that is, with no value
		// while it has never been given any.
		int64_t bitrate{ 0 };
		std::optional<size_t> currentOption;

		uint8_t timesGivenAPoint{ 0 };
	};

	// A candidate with a single point to run at, which is what a Consumer of
	// audio, of a pipe or of a single stream is.
	class TestFixedCandidate : public RTC::BWE::BitrateAllocator::Candidate
	{
	public:
		TestFixedCandidate(std::string id, int64_t outputBitrate)
		  : id(std::move(id)), outputBitrate(outputBitrate)
		{
		}

	public:
		const std::string& GetBitrateAllocationId() const override
		{
			return this->id;
		}

		uint8_t GetBitrateWeight() const override
		{
			FAIL("a candidate with a single point must not be asked for its weight");

			return 0;
		}

		BitrateOptions GetBitrateOptions() const override
		{
			return { .options = { { .bitrate = this->outputBitrate, .layers = {} } }, .currentOption = 0 };
		}

		void SetBitrateOption(const BitrateOption& /*option*/) override
		{
			FAIL("a candidate with a single point must not be given any");
		}

	private:
		const std::string id;
		const int64_t outputBitrate;
	};

	// Three points a sender of three simulcast layers could run at.
	const std::vector<int64_t> candidateBitrates{ 100, 200, 400 };

	SECTION("two candidates that want the same get the same")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateBitrates);
		TestCandidate candidateB("b", /*weight*/ 1, candidateBitrates);

		candidateA.SetMaxPoint(2);
		candidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		// The last point costs 200, and once both have climbed to 200 what is left
		// pays for neither of them.
		REQUIRE(candidateA.GetBitrate() == 200);
		REQUIRE(candidateB.GetBitrate() == 200);
	}

	SECTION("the one with twice the weight climbs with what the other cannot afford")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 2, candidateBitrates);
		TestCandidate candidateB("b", /*weight*/ 1, candidateBitrates);

		candidateA.SetMaxPoint(2);
		candidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(700, { std::addressof(candidateA), std::addressof(candidateB) });

		// Both take their first point out of the opening round, and from there the
		// heavier one gets two thirds of what is left, which is what pays for the
		// 200 of its last point.
		REQUIRE(candidateA.GetBitrate() == 400);
		REQUIRE(candidateB.GetBitrate() == 200);
	}

	SECTION("what the application caps is left for the others")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateBitrates);
		TestCandidate candidateB("b", /*weight*/ 1, candidateBitrates);

		// The application only wants the lowest layer of the first one.
		candidateA.SetMaxPoint(0);
		candidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		// The capped one stops taking after its first point and the other one gets
		// all the way up, which half of what was left would not have paid for.
		REQUIRE(candidateA.GetBitrate() == 100);
		REQUIRE(candidateB.GetBitrate() == 400);
	}

	SECTION("a candidate that wants nothing is left out")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 0, candidateBitrates);
		TestCandidate candidateB("b", /*weight*/ 1, candidateBitrates);

		candidateA.SetMaxPoint(2);
		candidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		REQUIRE(candidateA.GetBitrate() == 0);
		REQUIRE(candidateA.GetTimesGivenAPoint() == 0);
		REQUIRE(candidateB.GetBitrate() == 400);
	}

	SECTION("the point only one of them can have does not always fall to the same one")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateBitrates);
		TestCandidate candidateB("b", /*weight*/ 1, candidateBitrates);

		candidateA.SetMaxPoint(2);
		candidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(600, { std::addressof(candidateA), std::addressof(candidateB) });

		REQUIRE(candidateA.GetBitrate() == 400);
		REQUIRE(candidateB.GetBitrate() == 200);

		// The next round starts the group one position further along, so this time
		// it is the other one that climbs.
		TestCandidate otherCandidateA("a", /*weight*/ 1, candidateBitrates);
		TestCandidate otherCandidateB("b", /*weight*/ 1, candidateBitrates);

		otherCandidateA.SetMaxPoint(2);
		otherCandidateB.SetMaxPoint(2);

		bitrateAllocator.Distribute(
		  600, { std::addressof(otherCandidateA), std::addressof(otherCandidateB) });

		REQUIRE(otherCandidateA.GetBitrate() == 200);
		REQUIRE(otherCandidateB.GetBitrate() == 400);
	}

	SECTION("what a candidate with a single point puts out comes off the top")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestFixedCandidate fixedCandidate("a", /*outputBitrate*/ 300);
		TestCandidate candidate("b", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);

		bitrateAllocator.Distribute(700, { std::addressof(fixedCandidate), std::addressof(candidate) });

		// Of the 700, the 300 of the one that cannot be moved are already spoken
		// for, and the 400 that are left pay for every point of the other one.
		REQUIRE(candidate.GetBitrate() == 400);
	}

	SECTION("nothing is given out when a single point already fills the downlink")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestFixedCandidate fixedCandidate("a", /*outputBitrate*/ 500);
		TestCandidate candidate("b", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);

		bitrateAllocator.Distribute(400, { std::addressof(fixedCandidate), std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 0);
	}

	SECTION("nothing is committed until the whole round is over")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);

		// It climbs three points within the same round, and what it ends at is the
		// last one and not whatever it passed through.
		bitrateAllocator.Distribute(400, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 400);
		REQUIRE(candidate.GetTimesGivenAPoint() == 1);
	}

	SECTION("climbing above the point it is running at costs the margin on top")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);
		candidate.SetCurrentOption(1);

		// The 200 of the point above costs 240 to climb to, so the 120 left after
		// paying the 100 it is already running at do not reach the 140 of the climb.
		bitrateAllocator.Distribute(220, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 100);
		REQUIRE(candidate.GetTimesGivenAPoint() == 0);
	}

	SECTION("climbing above the point it is running at happens once the margin is paid")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);
		candidate.SetCurrentOption(1);

		// With 240 there are 140 left after the 100 it is already running at, which
		// is what climbing to the 200 costs.
		bitrateAllocator.Distribute(240, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 200);
	}

	SECTION("one that has never been given a point pays no margin")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);

		// The very same 220 that leave a candidate running at 100 where it is take
		// a fresh one up to 200, since it is not climbing back to anything.
		bitrateAllocator.Distribute(220, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 200);
	}

	SECTION("one that was left sending nothing pays the margin to come back")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);
		candidate.SetCurrentOption(0);

		// Its lowest point costs 100 and climbing to it costs 120, which is what
		// keeps a candidate that was dropped from coming straight back.
		bitrateAllocator.Distribute(110, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 0);
		REQUIRE(candidate.GetTimesGivenAPoint() == 0);
	}

	SECTION("dropping to a cheaper point costs just its price")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateBitrates);

		candidate.SetMaxPoint(2);
		candidate.SetCurrentOption(3);

		// Coming down from 400 to 100 pays the 100 the point costs and no margin,
		// since the margin is only for going above where it is running.
		bitrateAllocator.Distribute(150, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 100);
	}
}
