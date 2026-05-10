#include "common.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/public/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
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
	// TODO: SCTP: Uncoment.
	// constexpr uint32_t OutgoingMessageId{ 42 };
	// InitialTsn is the first TSN that will be assigned. The TSN before it
	// (InitialTsn - 1) starts as ACKED in OutstandingData, matching dcsctp's
	// invariant that the initial state has the previous TSN already
	// cumulative-acked.
	constexpr uint32_t InitialTsn{ 10 };

	const RTC::SCTP::SctpOptions sctpOptions{ .mtu = Mtu };

	MockRetransmissionQueueListener queueListener;
	mocks::RTC::SCTP::MockAssociationListener associationListener;
	mocks::RTC::SCTP::MockSendQueue sendQueue;
	uint64_t nowMs{ 0 };
	mocks::MockShared shared(/*getTimeMs*/
	                         [&nowMs]()
	                         {
		                         return nowMs;
	                         });

	const std::unique_ptr<BackoffTimerHandleInterface> t3RtxTimerUniquePtr{ shared.CreateBackoffTimer(
		BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		  // No `listener` given on purpose.
		  .label               = "mock-sctp-t3-rtx",
		  .baseTimeoutMs       = sctpOptions.initialRtoMs,
		  .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		  .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		  .maxRestarts         = std::nullopt }) };

	auto* t3RtxTimer = t3RtxTimerUniquePtr.get();

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

	auto createDataToSend = [](
	                          uint32_t outgoingMessageId,
	                          uint16_t maxRetransmissions = RTC::SCTP::Types::MaxRetransmitsNoLimit)
	{
		return [outgoingMessageId, maxRetransmissions](uint64_t /*nowMs*/, size_t /*maxLength*/)
		{
			RTC::SCTP::UserData data(
			  /*streamId*/ 1,
			  /*ssn*/ 0,
			  /*mid*/ 0,
			  /*fsn*/ 0,
			  /*ppid*/ 53,
			  /*payload*/ { 0x01, 0x02, 0x03, 0x04 },
			  /*isBeginning*/ true,
			  /*isEnd*/ true,
			  /*isUnordered*/ false);

			RTC::SCTP::SendQueueInterface::DataToSend dataToSend(outgoingMessageId, std::move(data));

			dataToSend.maxRetransmissions = maxRetransmissions;

			return dataToSend;
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

	auto getSentPacketTSNs = [&nowMs](RTC::SCTP::RetransmissionQueue& queue, size_t maxLength = 10000)
	{
		std::vector<uint32_t> tsns;

		for (const auto& elem : queue.GetChunksToSend(nowMs, maxLength))
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
		    { InitialTsn - 1, RTC::SCTP::OutstandingData::State::ACKED },
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
		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());
	}

	SECTION("restarts T3-rtx timer on retransmit first outstanding TSN")
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

		// Starting time.
		nowMs = 100 * 1000; // 100 seconds.

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10, 11, 12 });

		// Ack 10, 12, after 100ms.
		nowMs += 100;

		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    Arwnd,
		    {
		      { 2, 2 },
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED  },
		    { 11, RTC::SCTP::OutstandingData::State::NACKED },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED  },
    });

		// Send 13.
		sendQueue.WillProduceOnce(createDataToSend(3))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 13 });

		// Ack 10, 12-13, after 100ms.
		nowMs += 100;

		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    Arwnd,
		    {
		      { 2, 3 },
    })
		    .get());

		// Send 14.
		sendQueue.WillProduceOnce(createDataToSend(4))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 14 });

		// Ack 10, 12-14, after 100 ms.
		nowMs += 100;

		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    Arwnd,
		    {
		      { 2, 4 },
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 11, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED               },
    });

		// This will trigger "fast retransmit" mode and only chunks 13 and 16 will
		// be resent right now. The send queue will not even be queried.
		sendQueue.ExpectProduceCalledTimes(0);

		REQUIRE(getTSNsForFastRetransmit(queue) == std::vector<uint32_t>{ 11 });

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED     },
    });

		// Verify that the timer was really restarted when fast-retransmitting. The
		// timeout is `sctpOptions.initialRtoMs`, so advance the time just before
		// that.
		nowMs += (sctpOptions.initialRtoMs - 1);

		auto* backoffTimer = shared.GetBackoffTimer("mock-sctp-t3-rtx");

		REQUIRE(backoffTimer);
		REQUIRE(backoffTimer->EvaluateHasExpired() == false);

		nowMs += 1;

		REQUIRE(backoffTimer->EvaluateHasExpired() == true);
	}

	SECTION("can only produce two packets but wants to send three")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10, 11 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("retransmits on T3-rtx expiry")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("limited retransmission only with RFC 3758 support")
	{
		auto queue = createQueue(/*supportsPartialReliability*/ false);

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		// Discard must NOT be called.
		sendQueue.ExpectDiscardCalledTimes(0);

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());
	}

	SECTION("limits retransmissions as UDP")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted (abandoned because
		// `maxRetransmissions: 0`).
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(getSentPacketTSNs(queue, 1000).empty());
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });
	}

	SECTION("limits retransmissions to three sends")
	{
		auto queue = createQueue();

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 3))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// `Discard()` must NOT be called for the first three retransmissions.
		sendQueue.ExpectDiscardCalledTimes(0);

		// Retransmission 1.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(queue.GetChunksToSend(nowMs, 1000).size() == 1);

		// Retransmission 2.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(queue.GetChunksToSend(nowMs, 1000).size() == 1);

		// Retransmission 3.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(queue.GetChunksToSend(nowMs, 1000).size() == 1);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		// Retransmission 4. Not allowed, chunk is abandoned.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		queue.HandleT3RtxTimerExpiry();

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);
		REQUIRE(queue.GetChunksToSend(nowMs, 1000).empty());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });
	}

	SECTION("retransmits when send buffer is full on T3-rtx expiry")
	{
		auto queue = createQueue();

		const size_t cwnd{ 1200 };

		queue.SetCwnd(cwnd);

		REQUIRE(queue.GetCwnd() == cwnd);
		REQUIRE(queue.GetUnackedPacketBytes() == 0);
		REQUIRE(queue.GetUnackedItems() == 0);

		const std::vector<uint8_t> payload(1000, 0x00);

		sendQueue
		  .WillProduceOnce(
		    [&payload](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, true, true, false));
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue, 1500) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
		REQUIRE(
		  queue.GetUnackedPacketBytes() == payload.size() + RTC::SCTP::DataChunk::DataChunkHeaderLength);
		REQUIRE(queue.GetUnackedItems() == 1);

		// Will force chunks to be retransmitted.
		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		// After T3 expiry in-flight counters are cleared.
		REQUIRE(queue.GetUnackedPacketBytes() == 0);
		REQUIRE(queue.GetUnackedItems() == 0);
		REQUIRE(getSentPacketTSNs(queue, 1500) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
		REQUIRE(
		  queue.GetUnackedPacketBytes() == payload.size() + RTC::SCTP::DataChunk::DataChunkHeaderLength);
		REQUIRE(queue.GetUnackedItems() == 1);
	}

	SECTION("produces valid FORWARD-TSN")
	{
		// Three middle fragments (no "E"), same message (outgoingMessageId: 42,
		// SSN: 42). `Discard()` returns true, placeholder TSN 13 created.
		// FORWARD-TSN newCumulativeTsn: 13, skippedStreams: { (streamId=1, ssn=42) }.

		auto queue = createQueue();

		// "B" — beginning.
		sendQueue
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Middle fragment.
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x05, 0x06, 0x07, 0x08 }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Another middle fragment (message not fully sent — no "E").
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x09, 0x0a, 0x0b, 0x0c }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Ack TSN 10, but the remaining are lost.
		queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		// T3 expiry: TSN 11, 12 abandoned. `Discard()` returns true, placeholder TSN 13.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ true);

		queue.HandleT3RtxTimerExpiry();

		// NOTE: TSN 13 represents the placeholder end fragment.
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 13, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);

);const 
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };

		const auto* forwardTsnChunk = queue.CreateForwardTsn(packet.get());

		REQUIRE(forwardTsnChunk);
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 13);
		REQUIRE(
		  forwardTsnChunk->GetSkippedStreams() == std::vector<RTC::SCTP::ForwardTsnChunk::SkippedStream>{
		                                            RTC::SCTP::ForwardTsnChunk::SkippedStream{ 1, 42 }
    });
	}

	SECTION("produces valid FORWARD-TSN when fully sent")
	{
		// Three fragments "B"/""/""E" (message fully sent). `Discard()` returns
		// false, no placeholder. FORWARD-TSN newCumulativeTsn=12.

		auto queue = createQueue();

		sendQueue
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x05, 0x06, 0x07, 0x08 }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // "E" — end fragment (message fully sent).
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 42, 0, 0, 53, { 0x09, 0x0a, 0x0b, 0x0c }, false, true, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Ack TSN 10, but the remaining are lost.
		queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		// T3 expiry: TSN 11, 12 abandoned. `Discard()` returns false, no placeholder.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };

		const auto* forwardTsnChunk = queue.CreateForwardTsn(packet.get());

		REQUIRE(forwardTsnChunk);
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 12);
		REQUIRE(
		  forwardTsnChunk->GetSkippedStreams() == std::vector<RTC::SCTP::ForwardTsnChunk::SkippedStream>{
		                                            RTC::SCTP::ForwardTsnChunk::SkippedStream{ 1, 42 }
    });
	}

	SECTION("produces valid I-FORWARD-TSN")
	{
		auto queue = createQueue(/*supportsPartialReliability*/ true, /*useMessageInterleaving*/ true);

		// Stream 1, ordered, outgoingMessageId=42, MID=42, "B".
		sendQueue
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 42, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 2, unordered, outgoingMessageId=43, MID=42, "B".
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(2, 0, 42, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, true);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(43, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 3, ordered, outgoingMessageId=44, MID=42, "B".
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(3, 0, 42, 0, 53, { 0x09, 0x0a, 0x0b, 0x0c }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(44, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 4, ordered, outgoingMessageId=45, MID=42, "B".
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(4, 0, 42, 0, 53, { 0x0d, 0x0e, 0x0f, 0x10 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(45, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue, 1000) == std::vector<uint32_t>{ 10, 11, 12, 13 });
		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// TSN 13 is acked via gap block; TSN 10-12 are nacked.
		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 4, 4 },
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED  },
		    { 10, RTC::SCTP::OutstandingData::State::NACKED },
		    { 11, RTC::SCTP::OutstandingData::State::NACKED },
		    { 12, RTC::SCTP::OutstandingData::State::NACKED },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED  },
    });

		// T3 expiry: TSN 10-12 abandoned. `Discard()` called 3 times (one per stream),
		// each returns true, placeholder TSNs 14, 15, 16.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ true);
		sendQueue.WillDiscardOnce(2, 43, /*returnValue*/ true);
		sendQueue.WillDiscardOnce(3, 44, /*returnValue*/ true);

		queue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    // Placeholder end fragments for streams 1, 2 and 3.
		    { 14, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 15, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 16, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);

		// I-FORWARD-TSN: newCumulativeTsn=12 (can't go past ACKED TSN 13).
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };

		const auto* iForwardTsnChunk1 = queue.CreateIForwardTsn(packet.get());

		REQUIRE(iForwardTsnChunk1);
		REQUIRE(iForwardTsnChunk1->GetNewCumulativeTsn() == 12);
		REQUIRE(
		  iForwardTsnChunk1->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::ForwardTsnChunk::SkippedStream>{
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 1, false, 42 },
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 2, true,  42 },
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 3, false, 42 }
    });

		// When TSN 13 is acked, the placeholder end fragments must be skipped too.
		// A receiver is more likely to ack TSN 13, but do it incrementally.
		queue.HandleReceivedSackChunk(nowMs, createSackChunk(12, Arwnd).get());

		sendQueue.ExpectDiscardCalledTimes(0);

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		queue.HandleReceivedSackChunk(nowMs, createSackChunk(13, Arwnd).get());

		REQUIRE(queue.ShouldSendForwardTsn(nowMs) == true);

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 15, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 16, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		packet.reset(RTC::SCTP::Packet::Factory(sctpCommon::FactoryBuffer, sctpOptions.mtu));

		const auto* iForwardTsnChunk2 = queue.CreateIForwardTsn(packet.get());

		REQUIRE(iForwardTsnChunk2);
		REQUIRE(iForwardTsnChunk2->GetNewCumulativeTsn() == 16);
		REQUIRE(
		  iForwardTsnChunk2->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::ForwardTsnChunk::SkippedStream>{
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 1, false, 42 },
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 2, true,  42 },
		    RTC::SCTP::IForwardTsnChunk::SkippedStream{ 3, false, 42 }
    });
	}

	SECTION("measure RTT")
	{
		auto queue = createQueue(/*supportsPartialReliability*/ true, /*useMessageInterleaving*/ true);

		sendQueue.WillProduceOnce(createDataToSend(0, /*maxRetranmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(queue) == std::vector<uint32_t>{ 10 });

		const uint64_t durationMs{ 123 };

		nowMs += durationMs;

		queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(queueListener.lastRttMs == durationMs);
	}

	SECTION("validate cumulative TSN at rest")
	{
		// Nothing outstanding. TSN 8 is below lastCumulativeTsnAck(9) -> rejected.
		// TSN 9 equals lastCumulativeTsnAck(9) -> accepted (no-op).
		// TSN 10 is above highestOutstandingTsn(9) -> rejected.

		auto queue = createQueue();

		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(8, Arwnd).get()) == false);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(9, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get()) == false);
	}

	SECTION("validate cumulative TSN ack on inflight data")
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

		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(8, Arwnd).get()) == false);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(9, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(11, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(12, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(13, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(14, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(15, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(16, Arwnd).get()) == true);
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(17, Arwnd).get()) == true);
		// TSN 18 has never been sent -> rejected.
		REQUIRE(queue.HandleReceivedSackChunk(nowMs, createSackChunk(18, Arwnd).get()) == false);
	}

	SECTION("handle gap ack blocks matching no inflight data")
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

		// Ack 9, 20-25. This is an invalid SACK Chunk, but should still be handled.
		queue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 11, 16 },
    })
		    .get());

		REQUIRE(
		  queue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 14, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 15, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 16, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 17, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	// TODO: SCTP: A lot of tests.
}
