#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

SCENARIO("SCTP ReassemblyQueue", "[sctp][reassemblyqueue]")
{
	// The default maximum length of the reassembly queue.
	constexpr size_t BufferLength{ 10000 };

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
		const RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

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
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
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
				    /*streamId*/ 1,
				    /*ssn*/ 0,
				    /*mid*/ 0,
				    /*fsn*/ 0,
				    /*ppid*/ 53,
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
					REQUIRE(messages[0].GetStreamId() == 1);
					REQUIRE(messages[0].GetPayloadProtocolId() == 53);
					REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(LongPayload));
				}
			}
		} while (std::ranges::next_permutation(tsns).found);
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
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
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
				const auto ssn = static_cast<uint16_t>(tsns[i] - 10);

				reassemblyQueue.AddData(
				  tsns[i],
				  RTC::SCTP::UserData(
				    /*streamId*/ 1,
				    /*ssn*/ ssn,
				    /*mid*/ 0,
				    /*fsn*/ 0,
				    /*ppid*/ 53,
				    /*payload*/ std::vector<uint8_t>(span.begin(), span.end()),
				    /*isBeginning*/ isBeginning,
				    /*isEnd*/ isEnd,
				    /*isUnordered*/ false));
			}

			REQUIRE(reassemblyQueue.HasMessages() == true);

			const auto& messages = flushMessages(reassemblyQueue);

			REQUIRE(messages.size() == 4);
			REQUIRE(messages[0].GetStreamId() == 1);
			REQUIRE(messages[0].GetPayloadProtocolId() == 53);
			REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(0, 4)));
			REQUIRE(messages[1].GetStreamId() == 1);
			REQUIRE(messages[1].GetPayloadProtocolId() == 53);
			REQUIRE_THAT(messages[1].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(4, 4)));
			REQUIRE(messages[2].GetStreamId() == 1);
			REQUIRE(messages[2].GetPayloadProtocolId() == 53);
			REQUIRE_THAT(messages[2].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(8, 4)));
			REQUIRE(messages[3].GetStreamId() == 1);
			REQUIRE(messages[3].GetPayloadProtocolId() == 53);
			REQUIRE_THAT(messages[3].GetPayload(), Catch::Matchers::RangeEquals(payload.subspan(12, 4)));
		} while (std::ranges::next_permutation(tsns).found);
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
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 4 }, false, false, false));
		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 5 }, false, false, false));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 8 }, false, false, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);

		// Lost and retransmitted.
		reassemblyQueue.AddData(11, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 2 }, false, false, false));

		reassemblyQueue.AddData(18, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 9 }, false, false, false));
		reassemblyQueue.AddData(19, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 10 }, false, false, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 10);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		// Bit "E".
		reassemblyQueue.AddData(
		  20, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 11, 12, 13, 14, 15, 16 }, false, true, false));

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(LongPayload));
	}

	SECTION("Forward-TSN remove unordered")
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
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 3 }, false, false, true));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 4 }, false, true, true));

		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 5 }, true, false, true));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 6 }, false, false, true));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 8 }, false, true, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 6);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(13, std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{});

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 3);

		// The second lost chunk comes, message is assembled.
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 7 }, false, false, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);
		REQUIRE(reassemblyQueue.HasMessages() == true);
	}

	SECTION("Fforward-TSN remove ordered")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ false);

		// First message.
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
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 4 }, false, true, false));

		// Second message.
		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 5 }, true, false, false));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 8 }, false, true, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(
		  13,
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { /*streamId*/ 1, /*ssn*/ 0 }
    });

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);

		// The lost chunk comes, but too late.

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(Message2Payload));
	}

	SECTION("Forward-TSN remove a lot ordered")
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
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 4 }, false, true, false));

		reassemblyQueue.AddData(14, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 5 }, true, false, false));
		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 1, 0, 0, 53, { 8 }, false, true, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(
		  13,
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { /*streamId*/ 1, /*ssn*/ 0 }
    });

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);

		// The lost chunk comes, but too late.

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(Message2Payload));
	}

	SECTION("single unordered chunk message in RFC 8260")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ true);

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

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);
		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(ShortPayload));
	}

	SECTION("two interleaved chunks")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ true);

		// Stream 1.
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
		    /*isEnd*/ false,
		    /*isUnordered*/ true));

		// Stream 2.
		reassemblyQueue.AddData(
		  11, RTC::SCTP::UserData(2, 0, 0, 0, 53, { 9, 10, 11, 12 }, true, false, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 8);

		// Stream 1.
		reassemblyQueue.AddData(
		  12, RTC::SCTP::UserData(1, 0, 0, 1, 53, { 5, 6, 7, 8 }, false, true, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 12);

		// Stream 2.
		reassemblyQueue.AddData(
		  13, RTC::SCTP::UserData(2, 0, 0, 1, 53, { 13, 14, 15, 16 }, false, true, true));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 16);
		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 2);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(MediumPayload1));
		REQUIRE(messages[1].GetStreamId() == 2);
		REQUIRE(messages[1].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[1].GetPayload(), Catch::Matchers::RangeEquals(MediumPayload2));
	}

	SECTION("unordered interleaved messages all permutations")
	{
		std::vector<size_t> idxs = { 0, 1, 2, 3, 4, 5 };

		const std::vector<uint32_t> tsns      = { 10, 11, 12, 13, 14, 15 };
		const std::vector<uint16_t> streamIds = { 1, 2, 1, 1, 2, 2 };
		const std::vector<uint32_t> fsns      = { 0, 0, 1, 2, 1, 2 };
		const std::span<const uint8_t> payload(SixBytePayload);

		do
		{
			RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ true);

			for (auto i : idxs)
			{
				const auto span        = payload.subspan(fsns[i] * 2, 2);
				const bool isBeginning = (fsns[i] == 0);
				const bool isEnd       = (fsns[i] == 2);

				reassemblyQueue.AddData(
				  tsns[i],
				  RTC::SCTP::UserData(
				    /*streamId*/ streamIds[i],
				    /*ssn*/ 0,
				    /*mid*/ 0,
				    /*fsn*/ fsns[i],
				    /*ppid*/ 53,
				    /*payload*/ std::vector<uint8_t>(span.begin(), span.end()),
				    /*isBeginning*/ isBeginning,
				    /*isEnd*/ isEnd,
				    /*isUnordered*/ true));
			}

			REQUIRE(reassemblyQueue.HasMessages() == true);

			const auto& messages = flushMessages(reassemblyQueue);

			REQUIRE(messages.size() == 2);

			// NOTE: These are unordered messages so during the test permutations
			// they can be generated in different order.
			for (const auto& message : messages)
			{
				REQUIRE((message.GetStreamId() == 1 || message.GetStreamId() == 2));
				REQUIRE(message.GetPayloadProtocolId() == 53);
				REQUIRE_THAT(message.GetPayload(), Catch::Matchers::RangeEquals(SixBytePayload));
			}
		} while (std::ranges::next_permutation(idxs).found);
	}

	SECTION("I-Forward-TSN remove a lot ordered")
	{
		RTC::SCTP::ReassemblyQueue reassemblyQueue(BufferLength, /*useMessageInterleaving*/ true);

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
		// NOTE: Second fragment (fsn=1) is lost.
		reassemblyQueue.AddData(12, RTC::SCTP::UserData(1, 0, 0, 2, 53, { 3 }, false, false, false));
		reassemblyQueue.AddData(13, RTC::SCTP::UserData(1, 0, 0, 3, 53, { 4 }, false, true, false));

		reassemblyQueue.AddData(15, RTC::SCTP::UserData(1, 0, 1, 0, 53, { 5 }, true, false, false));
		reassemblyQueue.AddData(16, RTC::SCTP::UserData(1, 0, 1, 1, 53, { 6 }, false, false, false));
		reassemblyQueue.AddData(17, RTC::SCTP::UserData(1, 0, 1, 2, 53, { 7 }, false, false, false));
		reassemblyQueue.AddData(18, RTC::SCTP::UserData(1, 0, 1, 3, 53, { 8 }, false, true, false));

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 7);
		REQUIRE(reassemblyQueue.HasMessages() == false);

		reassemblyQueue.HandleForwardTsn(
		  13,
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { /*unordered*/ false, /*streamId*/ 1, /*mid*/ 0 }
    });

		REQUIRE(reassemblyQueue.GetQueuedBytes() == 4);

		// The lost chunk comes, but too late.

		REQUIRE(reassemblyQueue.HasMessages() == true);

		const auto& messages = flushMessages(reassemblyQueue);

		REQUIRE(messages.size() == 1);
		REQUIRE(messages[0].GetStreamId() == 1);
		REQUIRE(messages[0].GetPayloadProtocolId() == 53);
		REQUIRE_THAT(messages[0].GetPayload(), Catch::Matchers::RangeEquals(Message2Payload));
	}
}
