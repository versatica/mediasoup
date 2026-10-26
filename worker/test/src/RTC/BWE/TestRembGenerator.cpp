#include "common.hpp"
#include "RTC/BWE/RembGenerator.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("BWE RembGenerator", "[bwe][rembgenerator]")
{
	struct TestRemb
	{
		int64_t bitrate;
		std::vector<uint32_t> ssrcs;
	};

	class TestRembGeneratorListener : public RTC::BWE::RembGenerator::Listener
	{
	public:
		void OnRembGeneratorSendPacket(
		  RTC::BWE::RembGenerator* /*rembGenerator*/, RTC::RTCP::FeedbackPsRembPacket* packet) override
		{
			this->rembs.push_back(TestRemb{ .bitrate = packet->GetBitrate(), .ssrcs = packet->GetSsrcs() });
		}

	public:
		std::vector<TestRemb> rembs;
	};

	// Instant the scenarios below start at, which is irrelevant other than for
	// being far from zero.
	constexpr int64_t BaseTimeMs{ 1000 };
	const std::vector<uint32_t> ssrcs{ 1, 2, 3 };

	SECTION("the first estimation is announced right away")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 12345);

		REQUIRE(listener.rembs.size() == 1);
		REQUIRE(listener.rembs.at(0).bitrate == 12345);
		REQUIRE(listener.rembs.at(0).ssrcs == ssrcs);
	}

	SECTION("an estimation that barely drops waits")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 12346);

		REQUIRE(listener.rembs.size() == 1);

		// Less than 3 % below the one announced, so it waits.
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 100, ssrcs, 12345);

		REQUIRE(listener.rembs.size() == 1);

		// Past the 200 ms, so it goes out even though it barely moved.
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 201, ssrcs, 12345);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 12345);
	}

	SECTION("an estimation that drops a lot is announced right away")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 2345);
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 1, ssrcs, 1234);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(0).bitrate == 2345);
		REQUIRE(listener.rembs.at(1).bitrate == 1234);
	}

	SECTION("an estimation that rises waits")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 1234);
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 100, ssrcs, 2345);

		REQUIRE(listener.rembs.size() == 1);

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 201, ssrcs, 2345);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 2345);
	}

	SECTION("setting a cap announces it")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1234);

		REQUIRE(listener.rembs.size() == 1);
		REQUIRE(listener.rembs.at(0).bitrate == 1234);
		// The cap says nothing about which streams it applies to.
		REQUIRE(listener.rembs.at(0).ssrcs.empty());
	}

	SECTION("a cap that cannot be expressed is ignored")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1000);

		REQUIRE(listener.rembs.size() == 1);

		// A REMB of zero is how the wire says there is no limit, so zero cannot be
		// a cap, and a negative one means nothing.
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 1000, 0);
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 2000, -1);

		REQUIRE(listener.rembs.size() == 1);

		// The cap that was set is still the one in force.
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 3000, ssrcs, 5000);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 1000);
	}

	SECTION("the lowest of the cap and the estimation is announced")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 1234);

		REQUIRE(listener.rembs.size() == 1);
		REQUIRE(listener.rembs.at(0).bitrate == 1234);

		// Nothing to tell, since what was just announced is already below the cap.
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 1, 4567);

		REQUIRE(listener.rembs.size() == 1);

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 201, ssrcs, 5678);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 4567);
	}

	SECTION("a cap that needs no telling is not told at the next packet either")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs, ssrcs, 1234);

		REQUIRE(listener.rembs.size() == 1);

		// Nothing to tell, since what was just announced is already below the cap.
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 1, 4567);

		REQUIRE(listener.rembs.size() == 1);

		// And nothing right behind that REMB either, which would undo the wait
		// between one REMB and the next.
		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 2);

		REQUIRE(listener.rembs.size() == 1);
	}

	SECTION("a cap is not announced while the estimation keeps going out")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1000);

		REQUIRE(listener.rembs.size() == 1);

		// Well past the interval the cap would be announced again at, but the
		// estimation went out in between and that one is no higher than the cap.
		rembGenerator.OnReceiveBitrateChanged(BaseTimeMs + 1400, ssrcs, 5000);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 1000);

		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 1501);

		REQUIRE(listener.rembs.size() == 2);

		// Once the estimation stops, the cap has to be told again.
		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 2902);

		REQUIRE(listener.rembs.size() == 3);
		REQUIRE(listener.rembs.at(2).bitrate == 1000);
	}

	SECTION("removing the cap is announced four times")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1000);

		REQUIRE(listener.rembs.size() == 1);

		// The first of the four goes out without waiting.
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 1, std::nullopt);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 0);

		int64_t nowMs = BaseTimeMs + 1;

		for (size_t idx{ 0 }; idx < 3; ++idx)
		{
			// Before the interval is up nothing goes out.
			rembGenerator.MaySendLimitationRembFeedback(nowMs + 1);

			REQUIRE(listener.rembs.size() == idx + 2);

			nowMs += 1501;

			rembGenerator.MaySendLimitationRembFeedback(nowMs);

			REQUIRE(listener.rembs.size() == idx + 3);
			REQUIRE(listener.rembs.back().bitrate == 0);
		}

		REQUIRE(listener.rembs.size() == 5);

		// The four are spent, and with no cap there is nothing left to announce.
		nowMs += 1501;

		rembGenerator.MaySendLimitationRembFeedback(nowMs);

		REQUIRE(listener.rembs.size() == 5);
	}

	SECTION("a cap in force is announced again")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1000);

		REQUIRE(listener.rembs.size() == 1);

		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 1500);

		REQUIRE(listener.rembs.size() == 1);

		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 1501);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 1000);
	}

	SECTION("a cap set again stops announcing that it was removed")
	{
		TestRembGeneratorListener listener;
		RTC::BWE::RembGenerator rembGenerator(std::addressof(listener));

		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs, 1000);
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 1, std::nullopt);

		REQUIRE(listener.rembs.size() == 2);
		REQUIRE(listener.rembs.at(1).bitrate == 0);

		// Three of the four were still owed, and they no longer say anything true.
		rembGenerator.SetMaxIncomingBitrate(BaseTimeMs + 2, 2000);

		REQUIRE(listener.rembs.size() == 3);
		REQUIRE(listener.rembs.at(2).bitrate == 2000);

		// What comes out once the interval is up is the cap, not one of the zeros
		// that were owed.
		rembGenerator.MaySendLimitationRembFeedback(BaseTimeMs + 2000);

		REQUIRE(listener.rembs.size() == 4);
		REQUIRE(listener.rembs.at(3).bitrate == 2000);
	}
}
