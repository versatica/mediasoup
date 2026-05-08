#include "common.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/public/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include "test/include/catch2Macros.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("SCTP RetransmissionQueue", "[sctp][retransmissionqueue]")
{
	sctpCommon::ResetBuffers();

	class MockRetransmissionQueueListener : public RTC::SCTP::RetransmissionQueue::Listener
	{
	public:
		void OnRetransmissionQueueNewRttMs(uint64_t rttMs) override
		{
			this->lastRttMs = rttMs;
		}

		void OnRetransmissionQueueClearRetransmissionCounter() override
		{
			++this->clearRetransmissionCounterCalls;
		}

	public:
		uint64_t lastRttMs{ 0 };
		size_t clearRetransmissionCounterCalls{ 0 };
	};

	constexpr uint32_t Arwnd{ 100000 };
	constexpr uint64_t Mtu{ 1191 };
	constexpr uint32_t OutgoingMessageId{ 42 };
	// InitialTsn is the first TSN that will be assigned. The TSN before it
	// (InitialTsn - 1) starts as ACKED in OutstandingData, matching dcsctp's
	// invariant that the initial state has the previous TSN already
	// cumulative-acked.
	constexpr uint32_t InitialTsn{ 10 };
	constexpr uint32_t PreviousTsn{ InitialTsn - 1 };

	const mocks::MockShared shared;
	const RTC::SCTP::SctpOptions sctpOptions{ .mtu = Mtu };

	MockRetransmissionQueueListener queueListener;
	mocks::RTC::SCTP::MockAssociationListener associationListener;
	mocks::RTC::SCTP::MockSendQueue sendQueue;
	uint64_t nowMs{ 0 };

	auto* t3RtxTimer = shared.CreateBackoffTimer(
	  BackoffTimerHandleInterface::BackoffTimerHandleOptions{
	    // No `listener` given on purpose.
	    .label               = "mock-sctp-t3-rtx",
	    .baseTimeoutMs       = sctpOptions.initialRtoMs,
	    .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
	    .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
	    .maxRestarts         = std::nullopt });

	auto createQueue = [&queueListener, &associationListener, &sendQueue, &t3RtxTimer, &sctpOptions](
	                     bool supportsPartialReliability = true, bool useMessageInterleaving = false)
	{
		return RTC::SCTP::RetransmissionQueue(
		  std::addressof(queueListener),
		  associationListener,
		  InitialTsn,
		  Arwnd,
		  sendQueue,
		  t3RtxTimer,
		  sctpOptions,
		  supportsPartialReliability,
		  useMessageInterleaving);
	};

	auto createDataToSend = [](uint32_t outgoingMessageId)
	{
		return [outgoingMessageId](uint64_t /*nowMs*/, size_t /*maxLength*/)
		{
			RTC::SCTP::UserData data(
			  1, 0, 0, 0, 53, { 1, 2, 3, 4 }, /*isBeginning*/ true, /*isEnd*/ true, /*isUnordered*/ false);

			return RTC::SCTP::SendQueueInterface::DataToSend(outgoingMessageId, std::move(data));
		};
	};

	auto createSackChunk = [](
	                         uint32_t tsn,
	                         uint32_t arwnd,
	                         const std::vector<RTC::SCTP::SackChunk::GapAckBlock>&& gapAckBlocks = {})
	{
		std::unique_ptr<RTC::SCTP::SackChunk> chunk{ RTC::SCTP::SackChunk::Factory(
			sctpCommon::FactoryBuffer, Mtu) };

		chunk->SetCumulativeTsnAck(tsn);
		chunk->SetAdvertisedReceiverWindowCredit(arwnd);

		for (const auto& gapAckBlock : gapAckBlocks)
		{
			chunk->AddAckBlock(gapAckBlock);
		}

		return chunk;
	};

	auto getTSNsForFastRetransmit = [](RTC::SCTP::RetransmissionQueue& queue)
	{
		std::vector<uint32_t> tsns;

		for (const auto& elem : queue.GetChunksForFastRetransmit(10000))
		{
			tsns.push_back(elem.first);
		}

		return tsns;
	};

	auto getSentPacketTSNs = [&nowMs](RTC::SCTP::RetransmissionQueue& queue)
	{
		std::vector<uint32_t> tsns;

		for (const auto& elem : queue.GetChunksToSend(nowMs, 10000))
		{
			tsns.push_back(elem.first);
		}

		return tsns;
	};

	SECTION("initial acked previous TSN")
	{
		auto queue = createQueue();

		REQUIRE(queue.GetUnackedItems() == 0);
		REQUIRE(queue.GetUnackedPacketBytes() == 0);
		REQUIRE(queue.GetNextTsn() == InitialTsn);
		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>>{
		    { PreviousTsn, RTC::SCTP::OutstandingData::State::ACKED },
    });
	}

	SECTION("send one chunk")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("send one chunk and ack")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10 });

		queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED },
    });
	}

	SECTION("send three chunks and ack two")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceOnce(createDataToSend(2))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10, 11, 12 });

		queue.HandleReceivedSackChunk(nowMs, createSackChunk(11, Arwnd).get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("ack with gap blocks from RFC 4960 section 334")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceOnce(createDataToSend(2))
		  .WillProduceOnce(createDataToSend(3))
		  .WillProduceOnce(createDataToSend(4))
		  .WillProduceOnce(createDataToSend(5))
		  .WillProduceOnce(createDataToSend(6))
		  .WillProduceOnce(createDataToSend(7))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    12,
		    Arwnd,
		    {
		      { 2, 3 },
          { 5, 5 }
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 12, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 13, RTC::SCTP::OutstandingData::State::NACKED },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 16, RTC::SCTP::OutstandingData::State::NACKED },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED  },
    });
	}

	SECTION("resend packet when nacked three times")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceOnce(createDataToSend(2))
		  .WillProduceOnce(createDataToSend(3))
		  .WillProduceOnce(createDataToSend(4))
		  .WillProduceOnce(createDataToSend(5))
		  .WillProduceOnce(createDataToSend(6))
		  .WillProduceOnce(createDataToSend(7))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		// Send more chunks, but leave some as gaps to force retransmission after
		// three NACKs.

		// Send TSN 18.
		sendQueue.WillProduceOnce(createDataToSend(8))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 18 });

		// Ack 12, 14-15, 17-18.
		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    12,
		    Arwnd,
		    {
		      { 2, 3 },
          { 5, 6 }
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 12, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 13, RTC::SCTP::OutstandingData::State::NACKED },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 16, RTC::SCTP::OutstandingData::State::NACKED },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 18, RTC::SCTP::OutstandingData::State::ACKED  },
    });

		// Send TSN 19.
		sendQueue.WillProduceOnce(createDataToSend(9))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 19 });

		// Ack 12, 14-15, 17-19.
		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    12,
		    Arwnd,
		    {
		      { 2, 3 },
          { 5, 7 }
    })
		    .get());

		// Send TSN 20.
		sendQueue.WillProduceOnce(createDataToSend(10))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 20 });

		// Ack 12, 14-15, 17-20.
		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    12,
		    Arwnd,
		    {
		      { 2, 3 },
          { 5, 8 }
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 12, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 13, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 16, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 18, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 19, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 20, RTC::SCTP::OutstandingData::State::ACKED               },
    });

		// This will trigger "fast retransmit" mode and only chunks 13 and 16 will
		// be resent right now. The send queue will not even be queried.
		sendQueue.ExpectProduceCalledTimes(0);

		REQUIRE(getTSNsForFastRetransmit(queue) == std::vector<uint32_t>{ 13, 16 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 16, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 18, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 19, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 20, RTC::SCTP::OutstandingData::State::ACKED     },
    });

		REQUIRE_WITH_MESSAGE(sendQueue.VerifyExpectations());
	}

	// TODO: SCTP: A lot of tests.
}
