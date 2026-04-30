#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include "RTC/SCTP/tx/OutstandingData.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("SCTP OutstandingData", "[sctp][outstandingdata]")
{
	sctpCommon::ResetBuffers();

	class DiscardFromSendQueueTester
	{
	public:
		void Prepare(bool returnValue)
		{
			if (this->prepared)
			{
				throw std::runtime_error("already prepared");
			}

			this->prepared    = true;
			this->returnValue = returnValue;
		}

		bool Called(uint16_t streamId, uint32_t outgoingMessageId)
		{
			if (!this->prepared)
			{
				return false;
			}

			this->callCount++;
			this->lastStreamId          = streamId;
			this->lastOutgoingMessageId = outgoingMessageId;

			return this->returnValue;
		}

		void Test(
		  size_t expectedCallCount, uint16_t expectedLastStreamId, uint32_t expectedLastOutgoingMessageId)
		{
			if (!this->prepared)
			{
				throw std::runtime_error("not prepared");
			}

			REQUIRE(this->callCount == expectedCallCount);
			REQUIRE(this->lastStreamId == expectedLastStreamId);
			REQUIRE(this->lastOutgoingMessageId == expectedLastOutgoingMessageId);

			// Reset.
			this->prepared              = false;
			this->callCount             = 0;
			this->lastStreamId          = 0;
			this->lastOutgoingMessageId = 0;
			this->returnValue           = false;
		}

	private:
		bool prepared{ false };
		size_t callCount{ 0 };
		uint16_t lastStreamId{ 0 };
		uint32_t lastOutgoingMessageId{ 0 };
		bool returnValue{ false };
	};

	constexpr uint64_t NowMs{ 42 };
	constexpr uint32_t MessageId{ 17 };

	RTC::SCTP::OutstandingData::UnwrappedTsn::Unwrapper unwrapper;
	DiscardFromSendQueueTester discardFromSendQueueTester;

	auto discardFromSendQueue =
	  [&discardFromSendQueueTester](uint16_t streamId, uint32_t outgoingMessageId)
	{
		return discardFromSendQueueTester.Called(streamId, outgoingMessageId);
	};

	RTC::SCTP::OutstandingData buffer(
	  /*dataChunkHeaderLength*/ RTC::SCTP::DataChunk::DataChunkHeaderLength,
	  /*lastCumulativeTsnAck*/ unwrapper.Unwrap(9),
	  discardFromSendQueue);

	SECTION("has initial state")
	{
		REQUIRE(buffer.IsEmpty() == true);
		REQUIRE(buffer.GetUnackedPayloadBytes() == 0);
		REQUIRE(buffer.GetUnackedPacketBytes() == 0);
		REQUIRE(buffer.GetUnackedItems() == 0);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 9);
		REQUIRE(buffer.GetNextTsn().Wrap() == 10);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 9);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9, RTC::SCTP::OutstandingData::State::ACKED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
		REQUIRE(buffer.ShouldSendForwardTsn() == false);
	}

	SECTION("insert chunk")
	{
		const auto tsn = buffer.Insert(
		  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		REQUIRE(tsn.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(tsn->Wrap() == 10);

		REQUIRE(buffer.IsEmpty() == false);
		REQUIRE(buffer.GetUnackedPayloadBytes() == 1);
		REQUIRE(buffer.GetUnackedItems() == 1);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 9);
		REQUIRE(buffer.GetNextTsn().Wrap() == 11);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 10);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("acks single chunk")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(10), {}, false);

		REQUIRE(
		  ackInfo.bytesAcked ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(ackInfo.highestTsnAcked.Wrap() == 10);
		REQUIRE(ackInfo.hasPacketLoss == false);

		REQUIRE(buffer.IsEmpty() == true);
		REQUIRE(buffer.GetUnackedPayloadBytes() == 0);
		REQUIRE(buffer.GetUnackedPacketBytes() == 0);
		REQUIRE(buffer.GetUnackedItems() == 0);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 10);
		REQUIRE(buffer.GetNextTsn().Wrap() == 11);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 10);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 10, RTC::SCTP::OutstandingData::State::ACKED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("acks previous chunk doesn't update")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		REQUIRE(buffer.IsEmpty() == false);
		REQUIRE(buffer.GetUnackedPayloadBytes() == 1);
		REQUIRE(buffer.GetUnackedItems() == 1);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 9);
		REQUIRE(buffer.GetNextTsn().Wrap() == 11);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 10);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("acks and nacks with gap-ack-blocks")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false), NowMs);
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false), NowMs);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab = {
			{ 2, 2 }
		};
		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(9), gab, false);

		REQUIRE(
		  ackInfo.bytesAcked ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(ackInfo.highestTsnAcked.Wrap() == 11);
		REQUIRE(ackInfo.hasPacketLoss == false);

		// TSN 10 is still outstanding.
		REQUIRE(buffer.GetUnackedPayloadBytes() == 1);
		REQUIRE(buffer.GetUnackedItems() == 1);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 9);
		REQUIRE(buffer.GetNextTsn().Wrap() == 12);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 11);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED  },
			{ 10, RTC::SCTP::OutstandingData::State::NACKED },
			{ 11, RTC::SCTP::OutstandingData::State::ACKED  },
		};
		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("nacks three times with same TSN doesn't retransmit")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false), NowMs);

		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false), NowMs);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED  },
			{ 10, RTC::SCTP::OutstandingData::State::NACKED },
			{ 11, RTC::SCTP::OutstandingData::State::ACKED  },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("nacks three times results in retransmission")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false), NowMs);

		buffer.Insert(
		  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false), NowMs);

		buffer.Insert(
		  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false), NowMs);

		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false), NowMs);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 2, 3 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab2, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 4 }
		};
		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(9), gab3, false);

		REQUIRE(
		  ackInfo.bytesAcked ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(ackInfo.highestTsnAcked.Wrap() == 13);
		REQUIRE(ackInfo.hasPacketLoss == true);

		REQUIRE(buffer.HasDataToBeRetransmitted() == true);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED               },
			{ 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
			{ 11, RTC::SCTP::OutstandingData::State::ACKED               },
			{ 12, RTC::SCTP::OutstandingData::State::ACKED               },
			{ 13, RTC::SCTP::OutstandingData::State::ACKED               },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);

		std::vector<std::pair<uint32_t, RTC::SCTP::UserData>> expectedChunksToBeFastRetransmitted;

		expectedChunksToBeFastRetransmitted.emplace_back(
		  10, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false));

		REQUIRE(buffer.GetChunksToBeFastRetransmitted(1000) == expectedChunksToBeFastRetransmitted);

		const std::vector<std::pair<uint32_t, RTC::SCTP::UserData>> expectedChunksToBeRetransmitted = {};

		REQUIRE(buffer.GetChunksToBeRetransmitted(1000) == expectedChunksToBeRetransmitted);
	}

	SECTION("nacks three times results in abandoning")
	{
		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 2, 3 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab2, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		discardFromSendQueueTester.Prepare(/*returnValue*/ false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 4 }
		};
		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(9), gab3, false);

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		REQUIRE(
		  ackInfo.bytesAcked ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(ackInfo.highestTsnAcked.Wrap() == 13);
		REQUIRE(ackInfo.hasPacketLoss == true);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetNextTsn().Wrap() == 14);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 11, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 13, RTC::SCTP::OutstandingData::State::ABANDONED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("nacks three times results in abandoning with placeholder")
	{
		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 2, 3 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab2, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		discardFromSendQueueTester.Prepare(/*returnValue*/ true);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 4 }
		};
		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(9), gab3, false);

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		REQUIRE(
		  ackInfo.bytesAcked ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(ackInfo.highestTsnAcked.Wrap() == 13);
		REQUIRE(ackInfo.hasPacketLoss == true);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetNextTsn().Wrap() == 15);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 11, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 13, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 14, RTC::SCTP::OutstandingData::State::ABANDONED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("expires chunk before it is inserted")
	{
		constexpr uint64_t ExpiresAtMs = NowMs + 1;

		auto tsn = buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  ExpiresAtMs);

		REQUIRE(tsn.has_value());

		tsn = buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  ExpiresAtMs);

		REQUIRE(tsn.has_value());

		discardFromSendQueueTester.Prepare(/*returnValue*/ false);

		tsn = buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false),
		  NowMs + 1,
		  /*maxRetransmits*/ RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  ExpiresAtMs);

		REQUIRE(!tsn.has_value());

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
		REQUIRE(buffer.GetLastCumulativeTsnAck().Wrap() == 9);
		REQUIRE(buffer.GetNextTsn().Wrap() == 13);
		REQUIRE(buffer.GetHighestOutstandingTsn().Wrap() == 12);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 11, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("can generate Forward TSN")
	{
		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		discardFromSendQueueTester.Prepare(/*returnValue*/ false);

		buffer.NackAll();

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 11, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		buffer.CreateForwardTsn(packet.get());

		const auto* forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 12);
	}

	SECTION("ack with gap blocks from RFC 9260 section 3.3.4")
	{
		for (int i{ 0 }; i < 8; ++i)
		{
			const bool isBeginning = (i == 0);
			const bool isEnd       = (i == 7);

			buffer.Insert(
			  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, isBeginning, isEnd, false), NowMs);
		}

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedStateAfterInsert = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 14, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 15, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 16, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 17, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedStateAfterInsert);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab = {
			{ 2, 3 },
      { 5, 5 }
		};

		buffer.HandleSack(unwrapper.Unwrap(12), gab, false);

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 12, RTC::SCTP::OutstandingData::State::ACKED  },
			{ 13, RTC::SCTP::OutstandingData::State::NACKED },
			{ 14, RTC::SCTP::OutstandingData::State::ACKED  },
			{ 15, RTC::SCTP::OutstandingData::State::ACKED  },
			{ 16, RTC::SCTP::OutstandingData::State::NACKED },
			{ 17, RTC::SCTP::OutstandingData::State::ACKED  },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("MeasureRtt()")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		buffer.Insert(
		  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs + 1);

		buffer.Insert(
		  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs + 2);

		constexpr uint64_t Duration{ 123 };

		const auto duration = buffer.MeasureRtt(NowMs + Duration, unwrapper.Unwrap(11));

		REQUIRE(duration.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(duration.value() == Duration - 1);
	}

	SECTION("must retransmit before getting nacked again")
	{
		// A chunk that has been nacked and scheduled for retransmission should not
		// get nacked again until it has actually been sent on the wire.
		for (int i{ 0 }; i <= 10; ++i)
		{
			const bool isBeginning = (i == 0);
			const bool isEnd       = (i == 10);

			buffer.Insert(
			  MessageId,
			  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, isBeginning, isEnd, false),
			  NowMs,
			  /*maxRetransmits*/ 1);
		}

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab1, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 2, 3 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab2, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 4 }
		};
		const auto ackInfo = buffer.HandleSack(unwrapper.Unwrap(9), gab3, false);

		REQUIRE(ackInfo.hasPacketLoss == true);

		REQUIRE(buffer.HasDataToBeRetransmitted() == true);

		// Don't retransmit yet. S simulate congestion window blocking it.
		// It still gets more SACKs indicating packet loss.
		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab4 = {
			{ 2, 5 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab4, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == true);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab5 = {
			{ 2, 6 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab5, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == true);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab6 = {
			{ 2, 7 }
		};
		const auto ackInfo2 = buffer.HandleSack(unwrapper.Unwrap(9), gab6, false);

		REQUIRE(ackInfo2.hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == true);

		// Now retransmit.
		const auto fastRetransmit = buffer.GetChunksToBeFastRetransmitted(1000);

		REQUIRE(fastRetransmit.size() == 1);
		REQUIRE(fastRetransmit[0].first == 10);
		REQUIRE(buffer.GetChunksToBeRetransmitted(1000).empty() == true);

		// TSN 10 is now lost again. It gets nacked and eventually abandoned.
		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab7 = {
			{ 2, 8 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab7, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab8 = {
			{ 2, 9 }
		};

		REQUIRE(buffer.HandleSack(unwrapper.Unwrap(9), gab8, false).hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab9 = {
			{ 2, 10 }
		};
		const auto ackInfo3 = buffer.HandleSack(unwrapper.Unwrap(9), gab9, false);

		REQUIRE(ackInfo3.hasPacketLoss == true);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
	}

	SECTION("lifecyle returns acked items in ack-info")
	{
		buffer.Insert(
		  1,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  RTC::SCTP::OutstandingData::ExpiresAtMsInfinite,
		  /*lifecycleId*/ 42);

		buffer.Insert(
		  2,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  RTC::SCTP::OutstandingData::ExpiresAtMsInfinite,
		  /*lifecycleId*/ 43);

		buffer.Insert(
		  3,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  RTC::SCTP::OutstandingData::MaxRetransmitsNoLimit,
		  RTC::SCTP::OutstandingData::ExpiresAtMsInfinite,
		  /*lifecycleId*/ 44);

		const auto ackInfo1 = buffer.HandleSack(unwrapper.Unwrap(11), {}, false);

		std::vector<uint64_t> expectedAckedLifecycleIds = { 42, 43 };

		REQUIRE(ackInfo1.ackedLifecycleIds == expectedAckedLifecycleIds);

		const auto ackInfo2 = buffer.HandleSack(unwrapper.Unwrap(12), {}, false);

		expectedAckedLifecycleIds = { 44 };

		REQUIRE(ackInfo2.ackedLifecycleIds == expectedAckedLifecycleIds);
	}

	SECTION("lifecyle returns abandoned nacked three times")
	{
		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }
		};
		const auto ackInfo1 = buffer.HandleSack(unwrapper.Unwrap(9), gab1, false);

		REQUIRE(ackInfo1.hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 2, 3 }
		};
		const auto ackInfo2 = buffer.HandleSack(unwrapper.Unwrap(9), gab2, false);

		REQUIRE(ackInfo2.hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		discardFromSendQueueTester.Prepare(/*returnValue*/ false);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 4 }
		};
		const auto ackInfo3 = buffer.HandleSack(unwrapper.Unwrap(9), gab3, false);

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		REQUIRE(ackInfo3.hasPacketLoss == true);

		const std::vector<uint64_t> expectedAbandonedLifecycleIds = {};

		REQUIRE(ackInfo3.abandonedLifecycleIds == expectedAbandonedLifecycleIds);
	}

	SECTION("lifecyle returns abandoned after T3 RTX timer expired")
	{
		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false),
		  NowMs,
		  /*maxRetransmits*/ 0);

		buffer.Insert(
		  MessageId,
		  RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0,
		  /*expiresAtMs*/ RTC::SCTP::OutstandingData::ExpiresAtMsInfinite,
		  /*lifecycleId*/ 42);

		std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
			{ 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);

		const std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 4 }
		};
		const auto ackInfo1 = buffer.HandleSack(unwrapper.Unwrap(9), gab1, false);

		REQUIRE(ackInfo1.hasPacketLoss == false);
		REQUIRE(buffer.HasDataToBeRetransmitted() == false);

		expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED  },
			{ 10, RTC::SCTP::OutstandingData::State::NACKED },
			{ 11, RTC::SCTP::OutstandingData::State::ACKED  },
			{ 12, RTC::SCTP::OutstandingData::State::ACKED  },
			{ 13, RTC::SCTP::OutstandingData::State::ACKED  },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);

		discardFromSendQueueTester.Prepare(/*returnValue*/ false);

		buffer.NackAll();

		discardFromSendQueueTester.Test(
		  /*expectedCallCount*/ 1,
		  /*expectedLastStreamId*/ 1,
		  /*expectedLastOutgoingMessageId*/ MessageId);

		expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 11, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED },
			{ 13, RTC::SCTP::OutstandingData::State::ABANDONED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		buffer.CreateForwardTsn(packet.get());

		const auto* forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 13);

		const auto ackInfo2 = buffer.HandleSack(unwrapper.Unwrap(13), {}, false);

		REQUIRE(ackInfo2.hasPacketLoss == false);

		const std::vector<uint64_t> expectedAbandonedLifecycleIds = { 42 };

		REQUIRE(ackInfo2.abandonedLifecycleIds == expectedAbandonedLifecycleIds);
	}

	SECTION("generates Forward TSN until next stream reset TSN")
	{
		// This test generates:
		// * Stream 1: TSN 10, 11, 12 <RESET>
		// * Stream 2: TSN 13, 14 <RESET>
		// * Stream 3: TSN 15, 16
		//
		// Then it expires chunk 12-15, and ensures that the generated FORWARD-TSN
		// only includes up till TSN 12 until the cum ack TSN has reached 12, and
		// then 13 and 14 are included, and then after the cum ack TSN has reached
		// 14, then 15 is included.
		//
		// What it shouldn't do, is to generate a FORWARD-TSN directly at the start
		// with new TSN=15, and setting [(sid=1, ssn=44), (sid=2, ssn=46),
		// (sid=3, ssn=47)], because that will confuse the receiver at TSN=17,
		// receiving SID=1, SSN=0 (it's reset!), expecting SSN to be 45.

		// TSN 10-12.
		buffer.Insert(
		  0,
		  RTC::SCTP::UserData(1, /*ssn*/ 42, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.Insert(
		  1,
		  RTC::SCTP::UserData(1, /*ssn*/ 43, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.Insert(
		  2,
		  RTC::SCTP::UserData(1, /*ssn*/ 44, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.BeginResetStreams();

		// TSN 13, 14.
		buffer.Insert(
		  3,
		  RTC::SCTP::UserData(2, /*ssn*/ 45, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.Insert(
		  4,
		  RTC::SCTP::UserData(2, /*ssn*/ 46, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.BeginResetStreams();

		// TSN 15, 16.
		buffer.Insert(
		  5,
		  RTC::SCTP::UserData(3, /*ssn*/ 47, 0, 0, 53, { 0x00 }, true, true, false),
		  NowMs,
		  /*maxRetransmits*/ 0);
		buffer.Insert(6, RTC::SCTP::UserData(3, /*ssn*/ 48, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		REQUIRE(buffer.ShouldSendForwardTsn() == false);

		buffer.HandleSack(unwrapper.Unwrap(11), {}, false);
		buffer.NackAll();

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 11, RTC::SCTP::OutstandingData::State::ACKED               },
			{ 12, RTC::SCTP::OutstandingData::State::ABANDONED           },
			{ 13, RTC::SCTP::OutstandingData::State::ABANDONED           },
			{ 14, RTC::SCTP::OutstandingData::State::ABANDONED           },
			{ 15, RTC::SCTP::OutstandingData::State::ABANDONED           },
			{ 16, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		buffer.CreateForwardTsn(packet.get());

		const auto* forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 12);

		std::vector<RTC::SCTP::ForwardTsnChunk::SkippedStream> expectedSkippedStreams{
			{ 1, 44 },
		};

		REQUIRE(forwardTsnChunk->GetSkippedStreams() == expectedSkippedStreams);

		// Ack 12, allowing a FORWARD-TSN that spans to TSN=14 to be created.
		buffer.HandleSack(unwrapper.Unwrap(12), {}, false);

		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		packet.reset(
		  RTC::SCTP::Packet::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)));

		buffer.CreateForwardTsn(packet.get());

		forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 14);

		expectedSkippedStreams = {
			{ 2, 46 },
		};

		REQUIRE(forwardTsnChunk->GetSkippedStreams() == expectedSkippedStreams);

		// Ack 13, allowing a FORWARD-TSN that spans to TSN=14 to be created.
		buffer.HandleSack(unwrapper.Unwrap(13), {}, false);

		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		packet.reset(
		  RTC::SCTP::Packet::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)));

		buffer.CreateForwardTsn(packet.get());

		forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 14);

		expectedSkippedStreams = {
			{ 2, 46 },
		};

		REQUIRE(forwardTsnChunk->GetSkippedStreams() == expectedSkippedStreams);

		// Ack 14, allowing a FORWARD-TSN that spans to TSN=15 to be created.
		buffer.HandleSack(unwrapper.Unwrap(14), {}, false);

		REQUIRE(buffer.ShouldSendForwardTsn() == true);

		packet.reset(
		  RTC::SCTP::Packet::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)));

		buffer.CreateForwardTsn(packet.get());

		forwardTsnChunk = packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();

		REQUIRE(forwardTsnChunk);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 15);

		expectedSkippedStreams = {
			{ 3, 47 },
		};

		REQUIRE(forwardTsnChunk->GetSkippedStreams() == expectedSkippedStreams);

		buffer.HandleSack(unwrapper.Unwrap(15), {}, false);

		REQUIRE(buffer.ShouldSendForwardTsn() == false);
	}

	SECTION("treats unacked payload bytes different from packet bytes")
	{
		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		REQUIRE(buffer.GetUnackedPayloadBytes() == 1);
		REQUIRE(
		  buffer.GetUnackedPacketBytes() ==
		  RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1));
		REQUIRE(buffer.GetUnackedItems() == 1);

		buffer.Insert(MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, true, true, false), NowMs);

		REQUIRE(buffer.GetUnackedPayloadBytes() == 2);
		REQUIRE(
		  buffer.GetUnackedPacketBytes() ==
		  2 * (RTC::SCTP::DataChunk::DataChunkHeaderLength + Utils::Byte::PadTo4Bytes<size_t>(1)));
		REQUIRE(buffer.GetUnackedItems() == 2);
	}

	SECTION("fast recovery increments nack count when cumulative TSN advances")
	{
		// This test verifies that the Fast Recovery retransmission rules are
		// correctly applied when the Cumulative TSN Ack point advances. RFC 9260
		// Section 7.2.4: "If an endpoint is in Fast Recovery and a SACK arrives that
		// advances the Cumulative TSN Ack Point, the miss indications are incremented
		// for all TSNs reported missing in the SACK."

		for (int i{ 10 }; i <= 16; ++i)
		{
			buffer.Insert(
			  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false), NowMs);
		}

		// SACK 1: Cumulative Ack = 10. Gap blocks for 12, 14, 16.
		// Missing: 11, 13, 15.
		// This marks 12, 14, 16 as Acked.
		// TSNs 11, 13, 15 get their 1st miss indication each.
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }, // TSN 12
			{ 4, 4 }, // TSN 14
			{ 6, 6 }  // TSN 16
		};

		buffer.HandleSack(unwrapper.Unwrap(10), gab1, /*isInFastRecovery*/ false);

		// SACK 2: Cumulative Ack advances to 11. Same gap blocks (12, 14, 16).
		// Endpoint is now in Fast Recovery (is_in_fast_recovery = true). Because the
		// Cumulative TSN Ack Point advanced from 10 to 11, 13 and 15 should get their
		// 2nd miss indication.
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 1, 1 }, // TSN 12
			{ 3, 3 }, // TSN 14
			{ 5, 5 }  // TSN 16
		};

		buffer.HandleSack(unwrapper.Unwrap(11), gab2, /*isInFastRecovery*/ true);

		// SACK 3: Cumulative Ack advances to 12.
		// Note: TSN 12 was already acked via gap block, so this just advances the
		// Cumulative Ack. 13 and 15 should get their 3rd miss indication and trigger
		// retransmission.
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab3 = {
			{ 2, 2 }, // TSN 14
			{ 4, 4 }  // TSN 16
		};

		buffer.HandleSack(unwrapper.Unwrap(12), gab3, /*isInFastRecovery*/ true);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 12, RTC::SCTP::OutstandingData::State::ACKED               },
			{ 13, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
			{ 14, RTC::SCTP::OutstandingData::State::ACKED               },
			{ 15, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
			{ 16, RTC::SCTP::OutstandingData::State::ACKED               },
		};

		REQUIRE(buffer.GetChunkStatesForTesting() == expectedState);
	}

	SECTION("nack between ack blocks does not access out of bounds")
	{
		for (int i{ 0 }; i < 5; ++i)
		{
			buffer.Insert(
			  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false), NowMs);
		}

		// Inject a malformed SACK where the GapAckBlock exceeds the number of
		// outstanding items, potentially triggering an OOB read/write.
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> malformedBlocks = {
			{ 1, 40000 },
		};

		buffer.HandleSack(unwrapper.Unwrap(10), malformedBlocks, /*isInFastRecovery*/ false);

		REQUIRE(buffer.HasDataToBeRetransmitted() == false);
	}

	SECTION("handles SACKs with out of bounds TSNs")
	{
		// Send chunks with TSNs 10, 11, 12, 13, 14, 15, 16
		for (int i{ 0 }; i < 7; ++i)
		{
			buffer.Insert(
			  MessageId, RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, false, false, false), NowMs);
		}

		// This NACKs TSN 11, 13, 15 (1st miss indication).
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab1 = {
			{ 2, 2 }, // TSN 12
			{ 4, 4 }, // TSN 14
			{ 6, 6 }  // TSN 16
		};

		buffer.HandleSack(unwrapper.Unwrap(10), gab1, /*isInFastRecovery*/ false);

		REQUIRE(buffer.GetUnackedItems() == 3);

		// The gap between block1-end (12) and block2-start (1011) causes
		// NackBetweenAckBlocks to loop TSN 13..1010, but only TSN 13..16 are valid.
		std::vector<RTC::SCTP::SackChunk::GapAckBlock> gab2 = {
			{ 1,    1     }, // TSN 12
			{ 1000, 60000 }  // TSN 1011..60011
		};

		buffer.HandleSack(unwrapper.Unwrap(11), gab2, /*isInFastRecovery*/ true);

		// Packet 11 has been acknowledged.
		REQUIRE(buffer.GetUnackedItems() == 2);
	}
}
