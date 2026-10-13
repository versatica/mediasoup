#include "common.hpp"
#include "RTC/BWE/BitrateAllocator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

SCENARIO("BWE BitrateAllocator", "[bwe][bitrateallocator]")
{
	// A candidate that can sit at any of the given bitrates and nowhere in
	// between, which is what a Consumer with its layers is.
	class TestCandidate : public RTC::BWE::BitrateAllocator::Candidate
	{
	public:
		TestCandidate(std::string id, uint8_t weight, std::vector<int64_t> steps)
		  : id(std::move(id)), weight(weight), steps(std::move(steps))
		{
		}

	public:
		const std::string& GetBitrateAllocationId() const override
		{
			return this->id;
		}

		bool IsBitrateAdaptable() const override
		{
			return true;
		}

		int64_t GetOutputBitrate() override
		{
			FAIL("an adaptable candidate must not be asked what it puts out");

			return 0;
		}

		uint8_t GetBitrateWeight() const override
		{
			return this->weight;
		}

		int64_t TakeBitrate(int64_t bitrate) override
		{
			// Already as high as the application lets it go.
			if (this->provisionalStep + 1 > this->maxStep)
			{
				return 0;
			}

			const int64_t cost = this->steps.at(this->provisionalStep + 1) - GetProvisionalBitrate();

			if (cost > bitrate)
			{
				return 0;
			}

			this->provisionalStep++;

			return cost;
		}

		void ApplyBitrateAllocation() override
		{
			this->step = this->provisionalStep;
		}

		// Bitrate it ended the round at.
		int64_t GetBitrate() const
		{
			return this->step < 0 ? 0 : this->steps.at(this->step);
		}

		// Cap the application put on it, as an index into the steps.
		void SetMaxStep(int64_t maxStep)
		{
			this->maxStep = maxStep;
		}

	private:
		int64_t GetProvisionalBitrate() const
		{
			return this->provisionalStep < 0 ? 0 : this->steps.at(this->provisionalStep);
		}

	private:
		const std::string id;
		const uint8_t weight;
		const std::vector<int64_t> steps;
		// Where it sits, and where it would sit if the round were committed now.
		// Below the first step until it is given anything at all.
		int64_t step{ -1 };
		int64_t provisionalStep{ -1 };
		int64_t maxStep{ 0 };
	};

	// A candidate that cannot be moved anywhere, which is what a Consumer of
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

		bool IsBitrateAdaptable() const override
		{
			return false;
		}

		int64_t GetOutputBitrate() override
		{
			return this->outputBitrate;
		}

		uint8_t GetBitrateWeight() const override
		{
			FAIL("a candidate that cannot adapt must not be asked for its weight");

			return 0;
		}

		int64_t TakeBitrate(int64_t /*bitrate*/) override
		{
			FAIL("a candidate that cannot adapt must not be offered anything");

			return 0;
		}

		void ApplyBitrateAllocation() override
		{
			FAIL("a candidate that cannot adapt has nothing to commit");
		}

	private:
		const std::string id;
		const int64_t outputBitrate;
	};

	// Three steps a sender of three simulcast layers could sit at.
	const std::vector<int64_t> candidateSteps{ 100, 200, 400 };

	SECTION("two candidates that want the same get the same")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateSteps);
		TestCandidate candidateB("b", /*weight*/ 1, candidateSteps);

		candidateA.SetMaxStep(2);
		candidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		// The last step costs 200, and once both have climbed to 200 what is left
		// pays for neither of them.
		REQUIRE(candidateA.GetBitrate() == 200);
		REQUIRE(candidateB.GetBitrate() == 200);
	}

	SECTION("the one with twice the weight climbs with what the other cannot afford")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 2, candidateSteps);
		TestCandidate candidateB("b", /*weight*/ 1, candidateSteps);

		candidateA.SetMaxStep(2);
		candidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(700, { std::addressof(candidateA), std::addressof(candidateB) });

		// Both take their first step out of the opening round, and from there the
		// heavier one gets two thirds of what is left, which is what pays for the
		// 200 of its last step.
		REQUIRE(candidateA.GetBitrate() == 400);
		REQUIRE(candidateB.GetBitrate() == 200);
	}

	SECTION("what the application caps is left for the others")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateSteps);
		TestCandidate candidateB("b", /*weight*/ 1, candidateSteps);

		// The application only wants the lowest layer of the first one.
		candidateA.SetMaxStep(0);
		candidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		// The capped one stops taking after its first step and the other one gets
		// all the way up, which half of what was left would not have paid for.
		REQUIRE(candidateA.GetBitrate() == 100);
		REQUIRE(candidateB.GetBitrate() == 400);
	}

	SECTION("a candidate that wants nothing is left out")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 0, candidateSteps);
		TestCandidate candidateB("b", /*weight*/ 1, candidateSteps);

		candidateA.SetMaxStep(2);
		candidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(500, { std::addressof(candidateA), std::addressof(candidateB) });

		REQUIRE(candidateA.GetBitrate() == 0);
		REQUIRE(candidateB.GetBitrate() == 400);
	}

	SECTION("the step only one of them can have does not always fall to the same one")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidateA("a", /*weight*/ 1, candidateSteps);
		TestCandidate candidateB("b", /*weight*/ 1, candidateSteps);

		candidateA.SetMaxStep(2);
		candidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(600, { std::addressof(candidateA), std::addressof(candidateB) });

		REQUIRE(candidateA.GetBitrate() == 400);
		REQUIRE(candidateB.GetBitrate() == 200);

		// The next round starts the group one position further along, so this time
		// it is the other one that climbs.
		TestCandidate otherCandidateA("a", /*weight*/ 1, candidateSteps);
		TestCandidate otherCandidateB("b", /*weight*/ 1, candidateSteps);

		otherCandidateA.SetMaxStep(2);
		otherCandidateB.SetMaxStep(2);

		bitrateAllocator.Distribute(
		  600, { std::addressof(otherCandidateA), std::addressof(otherCandidateB) });

		REQUIRE(otherCandidateA.GetBitrate() == 200);
		REQUIRE(otherCandidateB.GetBitrate() == 400);
	}

	SECTION("what a candidate cannot move comes off the top")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestFixedCandidate fixedCandidate("a", /*outputBitrate*/ 300);
		TestCandidate candidate("b", /*weight*/ 1, candidateSteps);

		candidate.SetMaxStep(2);

		bitrateAllocator.Distribute(700, { std::addressof(fixedCandidate), std::addressof(candidate) });

		// Of the 700, the 300 that cannot be moved are already spoken for, and the
		// 400 that are left pay for every step of the other one.
		REQUIRE(candidate.GetBitrate() == 400);
	}

	SECTION("nothing is given out when what cannot be moved already fills the downlink")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestFixedCandidate fixedCandidate("a", /*outputBitrate*/ 500);
		TestCandidate candidate("b", /*weight*/ 1, candidateSteps);

		candidate.SetMaxStep(2);

		bitrateAllocator.Distribute(400, { std::addressof(fixedCandidate), std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 0);
	}

	SECTION("nothing is committed until the whole round is over")
	{
		RTC::BWE::BitrateAllocator bitrateAllocator;

		TestCandidate candidate("a", /*weight*/ 1, candidateSteps);

		candidate.SetMaxStep(2);

		// It climbs two steps within the same round, and what it ends at is the
		// last one and not whatever it passed through.
		bitrateAllocator.Distribute(400, { std::addressof(candidate) });

		REQUIRE(candidate.GetBitrate() == 400);
	}
}
