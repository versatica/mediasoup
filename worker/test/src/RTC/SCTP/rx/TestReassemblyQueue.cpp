#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <iterator>
#include <span>
#include <vector>

SCENARIO("SCTP ReassemblyQueue", "[sctp][reassemblyqueue]")
{
	// The default maximum length of the reassembly queue.
	constexpr size_t BufferLength{ 10000 };

	constexpr uint16_t StreamID{ 1 };
	constexpr uint16_t SSN{ 0 };
	constexpr uint32_t MID{ 0 };
	constexpr uint32_t FSN{ 0 };
	constexpr uint32_t PPID{ 53 };

	constexpr std::array<uint8_t, 4> ShortPayload    = { 1, 2, 3, 4 };
	constexpr std::array<uint8_t, 4> Message2Payload = { 5, 6, 7, 8 };
	constexpr std::array<uint8_t, 6> SixBytePayload  = { 1, 2, 3, 4, 5, 6 };
	constexpr std::array<uint8_t, 8> MediumPayload1  = { 1, 2, 3, 4, 5, 6, 7, 8 };
	constexpr std::array<uint8_t, 8> MediumPayload2  = { 9, 10, 11, 12, 13, 14, 15, 16 };
	constexpr std::array<uint8_t, 16> LongPayload    = { 1, 2,  3,  4,  5,  6,  7,  8,
		                                                   9, 10, 11, 12, 13, 14, 15, 16 };

	auto flushMessages = [](RTC::SCTP::ReassemblyQueue& reassemblyQueue)
	{
		std::vector<RTC::SCTP::Message> messages;

		while (reassemblyQueue.HasMessages())
		{
			messages.emplace_back(reassemblyQueue.GetNextMessage().value());
		}

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 0);

		return messages;
	};

	SECTION("empty queue")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		REQUIRE(reassemblyQueue.HasMessages() == false);
		REQUIRE(reassemblyQueue.GetQueuedBytes() == 0);
	}

	SECTION("single unordered chunk message")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		reassemblyQueue.AddData(
		  /*tsn*/ 10,
		  RTC::SCTP::UserData(
		    /*streamId*/ 1,
		    /*ssn*/ 0,
		    /*mid*/ 0,
		    /*fsn*/ 0,
		    /*ppid*/ 53,
		    /*payload*/ { 1, 2, 3, 4 },
		    /*isBeginning*/ true,
		    /*isEnd*/ true,
		    /*isUnordered*/ true));

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == StreamID);
		REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(ShortPayload));
	}

	SECTION("large unordered chunk all permutations")
	{
		std::vector<uint32_t> tsns = { 10, 11, 12, 13 };

		const std::span<const uint8_t> payload(LongPayload);

		do
		{
			RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

			for (size_t i{ 0 }; i < tsns.size(); ++i)
			{
				const auto span        = payload.subspan((tsns[i] - 10) * 4, 4);
				const bool isBeginning = (tsns[i] == 10);
				const bool isEnd       = (tsns[i] == 13);

				reassemblyQueue.AddData(
				  tsns[i],
				  RTC::SCTP::UserData(
				    /*streamId*/ StreamID,
				    /*ssn*/ SSN,
				    /*mid*/ MID,
				    /*fsn*/ FSN,
				    /*ppid*/ PPID,
				    /*payload*/ std::vector<uint8_t>(span.begin(), span.end()),
				    /*isBeginning*/ isBeginning,
				    /*isEnd*/ isEnd,
				    /*isUnordered*/ false));

				if (i < 3)
				{
					REQUIRE(reassemblyQueue.HasMessages() == false);
				}
				else
				{
					REQUIRE(reassemblyQueue.HasMessages() == true);

					const auto& messages = flushMessages(reassemblyQueue);

					REQUIRE(messages.size() == 1);
					REQUIRE(messages[0].GetStreamId() == StreamID);
					REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
					REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(LongPayload));
				}
			}
		} while (std::next_permutation(std::begin(tsns), std::end(tsns)));
	}

	SECTION("single ordered chunk message")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		reassemblyQueue.AddData(
		  /*tsn*/ 10,
		  RTC::SCTP::UserData(
		    /*streamId*/ 1,
		    /*ssn*/ 0,
		    /*mid*/ 0,
		    /*fsn*/ 0,
		    /*ppid*/ 53,
		    /*payload*/ { 1, 2, 3, 4 },
		    /*isBeginning*/ true,
		    /*isEnd*/ true,
		    /*isUnordered*/ false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);
		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == StreamID);
		REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(ShortPayload));
	}

	SECTION("many small ordered messages")
	{
		std::vector<uint32_t> tsns = { 10, 11, 12, 13 };

		const std::span<const uint8_t> payload(LongPayload);

		do
		{
			RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

			for (size_t i{ 0 }; i < tsns.size(); ++i)
			{
				const auto span = payload.subspan((tsns[i] - 10) * 4, 4);
				const bool isBeginning{ true };
				const bool isEnd{ true };
				const uint16_t ssn = static_cast<uint16_t>(tsns[i] - 10);

				reassemblyQueue.AddData(
				  tsns[i],
				  RTC::SCTP::UserData(
				    /*streamId*/ StreamID,
				    /*ssn*/ ssn,
				    /*mid*/ MID,
				    /*fsn*/ FSN,
				    /*ppid*/ PPID,
				    /*payload*/ std::vector<uint8_t>(span.begin(), span.end()),
				    /*isBeginning*/ isBeginning,
				    /*isEnd*/ isEnd,
				    /*isUnordered*/ false));
			}

			REQUIRE(reassemblyQueue.HasMessages() == true);

			const auto& messages = flushMessages(reassemblyQueue);

			REQUIRE(messages.size() == 4);
			REQUIRE(messages[0].GetStreamId() == StreamID);
			REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
			REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(0, 4)));
			REQUIRE(messages[1].GetStreamId() == StreamID);
			REQUIRE(messages[1].GetPayloadProtocolId() == PPID);
			REQUIRE_THAT(messages[1].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(4, 4)));
			REQUIRE(messages[2].GetStreamId() == StreamID);
			REQUIRE(messages[2].GetPayloadProtocolId() == PPID);
			REQUIRE_THAT(messages[2].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(8, 4)));
			REQUIRE(messages[3].GetStreamId() == StreamID);
			REQUIRE(messages[3].GetPayloadProtocolId() == PPID);
			REQUIRE_THAT(messages[3].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(12, 4)));
		} while (std::next_permutation(std::begin(tsns), std::end(tsns)));
	}

	SECTION("retransmission in large ordered")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		reassemblyQueue.AddData(
		  /*tsn*/ 10,
		  RTC::SCTP::UserData(
		    /*streamId*/ 1,
		    /*ssn*/ 0,
		    /*mid*/ 0,
		    /*fsn*/ 0,
		    /*ppid*/ 53,
		    /*payload*/ { 1 },
		    /*isBeginning*/ true,
		    /*isEnd*/ false,
		    /*isUnordered*/ false));
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 2, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 3, 53, { 4 }, false, false, false));
		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 0, 0, 4, 53, { 5 }, false, false, false));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 0, 0, 5, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 0, 0, 6, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 0, 0, 7, 53, { 8 }, false, false, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);

		// Lost and retransmitted.
		reassemblyQueue.AddData(11, RTC::SCTP::UserData(1, 0, 0, 1, 53, { 2 }, false, false, false));

		reassemblyQueue.AddData(18, RTC::SCTP::UserData(1, 0, 0, 8, 53, { 9 }, false, false, false));
		reassemblyQueue.AddData(19, RTC::SCTP::UserData(1, 0, 0, 9, 53, { 10 }, false, false, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 10);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		// Bit "E".
		reassemblyQueue.AddData(
		  20, RTC::SCTP::UserData(1, 0, 0, 10, 53, { 11, 12, 13, 14, 15, 16 }, false, true, false));

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == StreamID);
		REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(LongPayload));
	}

	SECTION("forward TSN remove unordered")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		reassemblyQueue.AddData(
		  /*tsn*/ 10,
		  RTC::SCTP::UserData(
		    /*streamId*/ 1,
		    /*ssn*/ 0,
		    /*mid*/ 0,
		    /*fsn*/ 0,
		    /*ppid*/ 53,
		    /*payload*/ { 1 },
		    /*isBeginning*/ true,
		    /*isEnd*/ false,
		    /*isUnordered*/ true));
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 2, 53, { 3 }, false, false, true));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 3, 53, { 4 }, false, true, true));

		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 0, 0, 4, 53, { 5 }, true, false, true));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 0, 0, 5, 53, { 6 }, false, false, true));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 0, 0, 7, 53, { 8 }, false, true, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 6);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(13, std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{});

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 3);

		// The second lost chunk comes, message is assembled.
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 0, 0, 6, 53, { 7 }, false, false, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);
		REQUIRE(reassemblyQueue.HasMessages() == true);
	}

	SECTION("forward TSN remove ordered")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		reassemblyQueue.AddData(
		  /*tsn*/ 10,
		  RTC::SCTP::UserData(
		    /*streamId*/ 1,
		    /*ssn*/ 0,
		    /*mid*/ 0,
		    /*fsn*/ 0,
		    /*ppid*/ 53,
		    /*payload*/ { 1 },
		    /*isBeginning*/ true,
		    /*isEnd*/ false,
		    /*isUnordered*/ false));
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 2, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 3, 53, { 4 }, false, true, false));

		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 0, 0, 4, 53, { 5 }, true, false, false));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 0, 0, 5, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 0, 0, 6, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 0, 0, 7, 53, { 8 }, false, true, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(
		  13,
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { /*streamId*/ StreamID, /*ssn*/ SSN }
    });

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);

		// The lost chunk comes, but too late.
		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == StreamID);
		REQUIRE(messages[0].GetPayloadProtocolId() == PPID);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(Message2Payload));
	}

	// TODO: SCTP: More tests.
}
