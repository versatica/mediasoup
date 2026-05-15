#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/rx/InterleavedReassemblyStreams.hpp"
#include "RTC/SCTP/rx/ReassemblyStreamsInterface.hpp"
#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

SCENARIO("SCTP InterleavedReassemblyStreams", "[sctp][interleavedreassemblystreams]")
{
	class OnAssembledMessageTester
	{
	public:
		RTC::SCTP::ReassemblyStreamsInterface::OnAssembledMessage MakeCallback()
		{
			return [this](std::span<const RTC::SCTP::Types::UnwrappedTsn> tsns, RTC::SCTP::Message message)
			{
				this->callCount++;

				// Copy the span to a vector to survive the callback.
				this->lastTsns = std::vector<RTC::SCTP::Types::UnwrappedTsn>(tsns.begin(), tsns.end());
				this->lastMessages.push_back(std::move(message));
			};
		}

		bool GetCallCount(size_t expectedCallCount) const
		{
			return this->callCount == expectedCallCount;
		}

		std::vector<uint32_t> GetLastTsns() const
		{
			if (this->lastTsns.empty())
			{
				return {};
			}

			std::vector<uint32_t> tsns;

			tsns.reserve(this->lastTsns.size());

			for (const auto& tsn : this->lastTsns)
			{
				tsns.push_back(tsn.Wrap());
			}

			return tsns;
		}

		std::vector<RTC::SCTP::Message>& GetLastMessages()
		{
			return this->lastMessages;
		}

		bool CheckCallbackNotCalled() const
		{
			return (this->callCount == 0 && this->lastTsns.empty() && this->lastMessages.empty());
		}

		void Reset()
		{
			this->callCount = 0;
			this->lastTsns.clear();
			this->lastMessages.clear();
		}

	private:
		size_t callCount{ 0 };
		std::vector<RTC::SCTP::Types::UnwrappedTsn> lastTsns;
		std::vector<RTC::SCTP::Message> lastMessages;
	};

	RTC::SCTP::Types::UnwrappedTsn::Unwrapper tsn;

	auto getTsn = [&tsn](uint32_t value)
	{
		return tsn.Unwrap(value);
	};

	SECTION("add unordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  interleavedReassemblyStreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ true)) == 1);

		REQUIRE(
		  interleavedReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, true)) ==
		  3);

		REQUIRE(
		  interleavedReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, true)) == 2);

		// Adding the end fragment should make it empty again.
		REQUIRE(
		  interleavedReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 3, 53, { 0x07 }, false, true, true)) == -6);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1, 2, 3, 4 });
	}

	SECTION("add simple ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ false)) == 1);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, false)) ==
		  3);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 3, 53, { 0x07 }, false, true, false)) == -6);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1, 2, 3, 4 });
	}

	SECTION("add more complex ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ false)) == 1);

		// Captured without adding yet: mid=0, fsn=1 (second fragment of first message).
		RTC::SCTP::UserData lateData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, false);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 3, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: mid=1, single fragment.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 0, 1, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: mid=2, two fragments.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 0, 2, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 0, 2, 1, 53, { 0x07 }, false, true, false)) == 1);

		// Adding the late chunk completes mid=0, which triggers delivery of
		// mid=1 and mid=2 as well.
		// NOTE: clang-tidy doesn't understand that this is fine.
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move, bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(interleavedreassemblystreams.AddData(getTsn(2), std::move(lateData)) == -8);

		REQUIRE(tester.GetCallCount(3));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 6, 7 });
	}

	SECTION("delete unordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ true)) == 1);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, true)) ==
		  3);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, true)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.HandleForwardTsn(
		    getTsn(3),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      { /*unordered*/ true,
           /*streamId*/ 1,
           /*mid*/ 0 }
    }) == 6);
	}

	SECTION("delete simple ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ false)) == 1);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, false)) ==
		  3);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.HandleForwardTsn(
		    getTsn(3),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      { /*unordered*/ false,
           /*streamId*/ 1,
           /*mid*/ 0 }
    }) == 6);
	}

	SECTION("delete many ordered messages returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ false)) == 1);

		// mid=0 middle fragment (not added).
		// RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, false)

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 3, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: mid=1.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 0, 1, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: mid=2.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 0, 2, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 0, 2, 1, 53, { 0x07 }, false, true, false)) == 1);

		// Expire all three messages (skip through mid=2 inclusive).
		REQUIRE(
		  interleavedreassemblystreams.HandleForwardTsn(
		    getTsn(8),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      { /*unordered*/ false,
           /*streamId*/ 1,
           /*mid*/ 2 }
    }) == 8);
	}

	SECTION("delete ordered message delivers two returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ false,
		      /*isUnordered*/ false)) == 1);

		// mid=0 middle fragment (not added).
		// RTC::SCTP::UserData(1, 0, 0, 1, 53, { 0x02, 0x03, 0x04 }, false, false, false)

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 2, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 3, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: mid=1.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 0, 1, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: mid=2.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 0, 2, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 0, 2, 1, 53, { 0x07 }, false, true, false)) == 1);

		// Expire the first message (mid=0). The following two (mid=1 and mid=2) are
		// delivered.
		REQUIRE(
		  interleavedreassemblystreams.HandleForwardTsn(
		    getTsn(4),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      { /*unordered*/ false,
           /*streamId*/ 1,
           /*mid*/ 0 }
    }) == 8);
	}

	SECTION("can delete first ordered message")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		// Not received: mid=0, sid=1, tsn=1 (BE, single-chunk message).
		// Deleted via I-FORWARD-tsn: sid=1, mid=0.
		REQUIRE(
		  interleavedreassemblystreams.HandleForwardTsn(
		    getTsn(1),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      { /*unordered*/ false,
           /*streamId*/ 1,
           /*mid*/ 0 }
    }) == 0);

		// Receive mid=1 (next after the skipped mid=0): should be delivered immediately.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(2),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 1,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x02, 0x03, 0x04 },
		      /*isBeginning*/ true,
		      /*isEnd*/ true,
		      /*isUnordered*/ false)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 2 });

		auto& lastMessages = tester.GetLastMessages();

		REQUIRE(lastMessages.size() == 1);
		REQUIRE(std::move(lastMessages[0]).ReleasePayload() == std::vector<uint8_t>{ 0x02, 0x03, 0x04 });
	}

	SECTION("can reassemble fast path unordered")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		// Each chunk is a complete single-fragment message (BE), delivered immediately.
		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(1),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 0,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x01 },
		      /*isBeginning*/ true,
		      /*isEnd*/ true,
		      /*isUnordered*/ true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1 });

		auto& lastMessages1 = tester.GetLastMessages();

		REQUIRE(lastMessages1.size() == 1);
		REQUIRE(std::move(lastMessages1[0]).ReleasePayload() == std::vector<uint8_t>{ 0x01 });

		tester.Reset();

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 1, 0, 53, { 0x03 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 3 });

		auto& lastMessages2 = tester.GetLastMessages();

		REQUIRE(lastMessages2.size() == 1);
		REQUIRE(std::move(lastMessages2[0]).ReleasePayload() == std::vector<uint8_t>{ 0x03 });

		tester.Reset();

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 2, 0, 53, { 0x02 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 2 });

		auto& lastMessages3 = tester.GetLastMessages();

		REQUIRE(lastMessages3.size() == 1);
		REQUIRE(std::move(lastMessages3[0]).ReleasePayload() == std::vector<uint8_t>{ 0x02 });

		tester.Reset();

		REQUIRE(
		  interleavedreassemblystreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 3, 0, 53, { 0x04 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 4 });

		auto& lastMessages4 = tester.GetLastMessages();

		REQUIRE(lastMessages4.size() == 1);
		REQUIRE(std::move(lastMessages4[0]).ReleasePayload() == std::vector<uint8_t>{ 0x04 });
	}

	SECTION("can reassemble fast path ordered")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::InterleavedReassemblyStreams interleavedreassemblystreams(tester.MakeCallback());

		RTC::SCTP::UserData data1(1, 0, 0, 0, 53, { 0x01 }, true, true, false);
		RTC::SCTP::UserData data2(1, 0, 1, 0, 53, { 0x02 }, true, true, false);
		RTC::SCTP::UserData data3(1, 0, 2, 0, 53, { 0x03 }, true, true, false);
		RTC::SCTP::UserData data4(1, 0, 3, 0, 53, { 0x04 }, true, true, false);

		// tsn(1)/mid=0: delivered immediately (nextMid=0).
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move, bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(interleavedreassemblystreams.AddData(getTsn(1), std::move(data1)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1 });

		auto& lastMessages1 = tester.GetLastMessages();

		REQUIRE(lastMessages1.size() == 1);
		REQUIRE(std::move(lastMessages1[0]).ReleasePayload() == std::vector<uint8_t>{ 0x01 });

		tester.Reset();

		// tsn(3)/mid=2: buffered (nextMid=1).
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move, bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(interleavedreassemblystreams.AddData(getTsn(3), std::move(data3)) == 1);

		REQUIRE(tester.CheckCallbackNotCalled());

		// tsn(2)/mid=1: completes mid=1, then mid=2 is also delivered.
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move, bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(interleavedreassemblystreams.AddData(getTsn(2), std::move(data2)) == -1);

		REQUIRE(tester.GetCallCount(2));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 3 });

		auto& lastMessages2 = tester.GetLastMessages();

		REQUIRE(lastMessages2.size() == 2);
		REQUIRE(std::move(lastMessages2[0]).ReleasePayload() == std::vector<uint8_t>{ 0x02 });
		REQUIRE(std::move(lastMessages2[1]).ReleasePayload() == std::vector<uint8_t>{ 0x03 });

		tester.Reset();

		// tsn(4)/mid=3: delivered immediately (nextMid=3).
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.Move, bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(interleavedreassemblystreams.AddData(getTsn(4), std::move(data4)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 4 });

		auto& lastMessages3 = tester.GetLastMessages();

		REQUIRE(lastMessages3.size() == 1);
		REQUIRE(std::move(lastMessages3[0]).ReleasePayload() == std::vector<uint8_t>{ 0x04 });
	}
}
