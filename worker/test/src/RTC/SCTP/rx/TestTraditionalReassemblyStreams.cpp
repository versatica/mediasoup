#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/rx/ReassemblyStreamsInterface.hpp"
#include "RTC/SCTP/rx/TraditionalReassemblyStreams.hpp"
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <vector>

SCENARIO("SCTP TraditionalReassemblyStreams", "[sctp][traditionalreassemblystreams]")
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

				if (this->lastMessage.has_value())
				{
					this->penultimateMessage = this->lastMessage.value().Clone();
				}
				else
				{
					this->penultimateMessage.reset();
				}

				this->lastMessage = std::move(message);
			};
		}

		bool GetCallCount(size_t expectedCallCount) const
		{
			return this->callCount == expectedCallCount;
		}

		std::vector<uint32_t> GetLastTsns() const
		{
			if (!this->lastTsns.has_value())
			{
				return {};
			}

			std::vector<uint32_t> tsns;

			tsns.reserve(this->lastTsns->size());

			for (const auto& tsn : this->lastTsns.value())
			{
				tsns.push_back(tsn.Wrap());
			}

			return tsns;
		}

		std::optional<RTC::SCTP::Message>& GetLastMessage()
		{
			return this->lastMessage;
		}

		std::optional<RTC::SCTP::Message>& GetPenultimateMessage()
		{
			return this->penultimateMessage;
		}

		bool CheckCallbackNotCalled() const
		{
			return (this->callCount == 0 && !this->lastTsns.has_value() && !this->lastMessage.has_value());
		}

		void Reset()
		{
			this->callCount = 0;
			this->lastTsns.reset();
			this->lastMessage.reset();
			this->penultimateMessage.reset();
		}

	private:
		size_t callCount{ 0 };
		std::optional<std::vector<RTC::SCTP::Types::UnwrappedTsn>> lastTsns;
		std::optional<RTC::SCTP::Message> lastMessage;
		std::optional<RTC::SCTP::Message> penultimateMessage;
	};

	RTC::SCTP::Types::UnwrappedTsn::Unwrapper tsn;

	auto getTsn = [&tsn](uint32_t value)
	{
		return tsn.Unwrap(value);
	};

	SECTION("add unordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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
		  traditionalReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, true)) ==
		  3);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, true)) == 2);

		// Adding the end fragment should make it empty again.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x07 }, false, true, true)) == -6);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1, 2, 3, 4 });
	}

	SECTION("add simple ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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
		  traditionalReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, false)) ==
		  3);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x07 }, false, true, false)) == -6);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1, 2, 3, 4 });
	}

	SECTION("add more complex ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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

		// Captured without adding yet: ssn=0, middle fragment of the first message.
		RTC::SCTP::UserData lateData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, false);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: ssn=1.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 1, 0, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: ssn=2.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Adding the late chunk completes ssn=0, which triggers delivery of ssn=1
		// and ssn=2 as well.
		REQUIRE(traditionalReassemblyStreams.AddData(getTsn(2), std::move(lateData)) == -8);

		REQUIRE(tester.GetCallCount(3));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 6, 7 });
	}

	SECTION("delete unordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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
		  traditionalReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, true)) ==
		  3);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, true)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.HandleForwardTsn(
		    getTsn(3), std::span<const RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{}) == 6u);
	}

	SECTION("delete simple ordered message returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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
		  traditionalReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, false)) ==
		  3);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.HandleForwardTsn(
		    getTsn(3),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      RTC::SCTP::AnyForwardTsnChunk::SkippedStream{ /*streamId*/ 1, /*ssn*/ 0 }
    }) == 6);
	}

	SECTION("delete many ordered messages returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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

		// ssn=0 middle fragment (not added, consumed to advance the generator).
		// RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, false)

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: ssn=1.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 1, 0, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: ssn=2.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Expire all three messages (skip through ssn=2 inclusive).
		REQUIRE(
		  traditionalReassemblyStreams.HandleForwardTsn(
		    getTsn(8),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      RTC::SCTP::AnyForwardTsnChunk::SkippedStream{ /*streamId*/ 1, /*ssn*/ 2 }
    }) == 8);
	}

	SECTION("delete ordered message delivers two returns correct size")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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

		// ssn=0 middle fragment (not added, consumed to advance the generator).
		// RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02, 0x03, 0x04 }, false, false, false)

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x05, 0x06 }, false, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Second message: ssn=1.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(5), RTC::SCTP::UserData(1, 1, 0, 0, 53, { 0x01 }, true, true, false)) == 1);

		// Third message: ssn=2.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(6), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x05, 0x06 }, true, false, false)) == 2);

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(7), RTC::SCTP::UserData(1, 2, 0, 0, 53, { 0x07 }, false, true, false)) == 1);

		// Expire the first message (ssn=0). The following two (ssn=1 and ssn=2) are
		// delivered.
		REQUIRE(
		  traditionalReassemblyStreams.HandleForwardTsn(
		    getTsn(4),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      RTC::SCTP::AnyForwardTsnChunk::SkippedStream{ /*streamId*/ 1, /*ssn*/ 0 }
    }) == 8);
	}

	SECTION("can delete first ordered message")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		// Not received: ssn=0, SID=1, TSN=1 (BE, single-chunk message).
		// Consumed here only to advance the ssn counter conceptually.

		// Deleted via FORWARD-TSN: SID=1, SSN=0.
		REQUIRE(
		  traditionalReassemblyStreams.HandleForwardTsn(
		    getTsn(1),
		    std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		      RTC::SCTP::AnyForwardTsnChunk::SkippedStream{ /*streamId*/ 1, /*ssn*/ 0 }
    }) == 0);

		// Receive ssn=1 (next after the skipped ssn=0): should be delivered immediately.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(2),
		    RTC::SCTP::UserData(
		      /*streamId*/ 1,
		      /*ssn*/ 1,
		      /*mid*/ 0,
		      /*fsn*/ 0,
		      /*ppid*/ 53,
		      /*payload*/ { 0x02, 0x03, 0x04 },
		      /*isBeginning*/ true,
		      /*isEnd*/ true,
		      /*isUnordered*/ false)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 2 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() ==
		  std::vector<uint8_t>{ 0x02, 0x03, 0x04 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);
	}

	SECTION("can reassemble fast path unordered")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		// Each chunk is a complete single-fragment message (BE), delivered immediately.
		REQUIRE(
		  traditionalReassemblyStreams.AddData(
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
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x01 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);

		tester.Reset();

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(3), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x03 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 3 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x03 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);

		tester.Reset();

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(2), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x02 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 2 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x02 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);

		tester.Reset();

		REQUIRE(
		  traditionalReassemblyStreams.AddData(
		    getTsn(4), RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x04 }, true, true, true)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 4 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x04 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);
	}

	SECTION("can reassemble fast path ordered")
	{
		OnAssembledMessageTester tester;
		RTC::SCTP::TraditionalReassemblyStreams traditionalReassemblyStreams(tester.MakeCallback());

		RTC::SCTP::UserData data1(1, 0, 0, 0, 53, { 0x01 }, true, true, false);
		RTC::SCTP::UserData data2(1, 1, 0, 0, 53, { 0x02 }, true, true, false);
		RTC::SCTP::UserData data3(1, 2, 0, 0, 53, { 0x03 }, true, true, false);
		RTC::SCTP::UserData data4(1, 3, 0, 0, 53, { 0x04 }, true, true, false);

		// tsn(1)/ssn=0: delivered immediately (nextSsn=0).
		REQUIRE(traditionalReassemblyStreams.AddData(getTsn(1), std::move(data1)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 1 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x01 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);

		tester.Reset();

		// tsn(3)/ssn=2: buffered (nextSsn=1).
		REQUIRE(traditionalReassemblyStreams.AddData(getTsn(3), std::move(data3)) == 1);

		REQUIRE(tester.CheckCallbackNotCalled());

		// tsn(2)/ssn=1: completes ssn=1, then ssn=2 is also delivered.
		REQUIRE(traditionalReassemblyStreams.AddData(getTsn(2), std::move(data2)) == -1);

		REQUIRE(tester.GetCallCount(2));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 3 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x03 });
		// Notice that here we got message ssn=2 before last message ssn=3.
		REQUIRE(tester.GetPenultimateMessage().has_value());
		REQUIRE(
		  std::move(tester.GetPenultimateMessage().value()).ReleasePayload() ==
		  std::vector<uint8_t>{ 0x02 });

		tester.Reset();

		// tsn(4)/ssn=3: delivered immediately (nextSsn=3).
		REQUIRE(traditionalReassemblyStreams.AddData(getTsn(4), std::move(data4)) == 0);

		REQUIRE(tester.GetCallCount(1));
		REQUIRE(tester.GetLastTsns() == std::vector<uint32_t>{ 4 });
		REQUIRE(tester.GetLastMessage().has_value());
		REQUIRE(
		  std::move(tester.GetLastMessage().value()).ReleasePayload() == std::vector<uint8_t>{ 0x04 });
		REQUIRE(tester.GetPenultimateMessage().has_value() == false);
	}
}
