#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include "Utils.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include "test/include/catch2Macros.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
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

	class MockBackoffTimerHandleListener : public BackoffTimerHandleInterface::Listener
	{
		/* Pure virtual methods inherited from BackoffTimerHandleInterface::Listener. */
	public:
		void OnBackoffTimer(
		  BackoffTimerHandleInterface* /*backoffTimer*/, uint64_t& /*baseTimeoutMs*/, bool& /*stop*/) override
		{
		}
	};

	constexpr uint32_t Arwnd{ 100000 };
	constexpr uint64_t Mtu{ 1191 };
	// InitialTsn is the first TSN that will be assigned. The TSN before it
	// (InitialTsn - 1) starts as ACKED in OutstandingData, matching dcsctp's
	// invariant that the initial state has the previous TSN already
	// cumulative-acked.
	constexpr uint32_t InitialTsn{ 10 };

	const RTC::SCTP::SctpOptions sctpOptions{ .mtu = Mtu };

	MockRetransmissionQueueListener retransmissionQueueListener;
	MockBackoffTimerHandleListener backoffTimerHandleListener;
	mocks::RTC::SCTP::MockAssociationListener associationListener;
	mocks::RTC::SCTP::MockSendQueue sendQueue;
	uint64_t nowMs{ 10000 };
	mocks::MockShared shared(/*getTimeMs*/
	                         [&nowMs]()
	                         {
		                         return nowMs;
	                         });

	const std::unique_ptr<BackoffTimerHandleInterface> t3RtxTimerUniquePtr{ shared.CreateBackoffTimer(
		BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		  .listener            = std::addressof(backoffTimerHandleListener),
		  .label               = "mock-sctp-t3-rtx",
		  .baseTimeoutMs       = sctpOptions.initialRtoMs,
		  .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		  .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		  .maxRestarts         = std::nullopt }) };

	auto* t3RtxTimer = t3RtxTimerUniquePtr.get();

	auto createRetransmissionQueue =
	  [&retransmissionQueueListener, &associationListener, &sendQueue, &t3RtxTimer, &sctpOptions](
	    bool supportsPartialReliability = true, bool useMessageInterleaving = false)
	{
		return RTC::SCTP::RetransmissionQueue(
		  std::addressof(retransmissionQueueListener),
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

	auto createSackChunk = [sctpOptions](
	                         uint32_t tsn,
	                         uint32_t arwnd,
	                         const std::vector<RTC::SCTP::SackChunk::GapAckBlock>&& gapAckBlocks = {})
	{
		std::unique_ptr<RTC::SCTP::SackChunk> chunk{ RTC::SCTP::SackChunk::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };

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
		auto retransmissionQueue = createRetransmissionQueue();

		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetNextTsn() == InitialTsn);
		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>>{
		    { InitialTsn - 1, RTC::SCTP::OutstandingData::State::ACKED },
    });
	}

	SECTION("send one chunk")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("send one chunk and ack")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10 });

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED },
    });
	}

	SECTION("send three chunks and ack two")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceOnce(createDataToSend(2))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10, 11, 12 });

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(11, Arwnd).get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("ack with gap blocks from RFC 4960 section 334")
	{
		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		retransmissionQueue.HandleReceivedSackChunk(
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
		  retransmissionQueue.GetChunkStatesForTesting() ==
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
		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		// Send more chunks, but leave some as gaps to force retransmission after
		// three NACKs.

		// Send TSN 18.
		sendQueue.WillProduceOnce(createDataToSend(8))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 18 });

		// Ack 12, 14-15, 17-18.
		retransmissionQueue.HandleReceivedSackChunk(
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
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 19 });

		// Ack 12, 14-15, 17-19.
		retransmissionQueue.HandleReceivedSackChunk(
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

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 20 });

		// Ack 12, 14-15, 17-20.
		retransmissionQueue.HandleReceivedSackChunk(
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
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		REQUIRE(getTSNsForFastRetransmit(retransmissionQueue) == std::vector<uint32_t>{ 13, 16 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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
		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10, 11, 12 });

		// Ack 10, 12, after 100ms.
		nowMs += 100;

		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    Arwnd,
		    {
		      { 2, 2 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 13 });

		// Ack 10, 12-13, after 100ms.
		nowMs += 100;

		retransmissionQueue.HandleReceivedSackChunk(
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

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 14 });

		// Ack 10, 12-14, after 100 ms.
		nowMs += 100;

		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    Arwnd,
		    {
		      { 2, 4 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		REQUIRE(getTSNsForFastRetransmit(retransmissionQueue) == std::vector<uint32_t>{ 11 });

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceOnce(createDataToSend(1))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10, 11 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("retransmits on T3-rtx expiry")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("limited retransmission only with RFC 3758 support")
	{
		auto retransmissionQueue = createRetransmissionQueue(/*supportsPartialReliability*/ false);

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		// Discard must NOT be called.
		sendQueue.ExpectDiscardCalledTimes(0);

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());
	}

	SECTION("limits retransmissions as UDP")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Will force chunks to be retransmitted (abandoned because
		// `maxRetransmissions: 0`).
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000).empty());
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });
	}

	SECTION("limits retransmissions to three sends")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 3))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// `Discard()` must NOT be called for the first three retransmissions.
		sendQueue.ExpectDiscardCalledTimes(0);

		// Retransmission 1.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 1000).size() == 1);

		// Retransmission 2.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 1000).size() == 1);

		// Retransmission 3.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 1000).size() == 1);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		// Retransmission 4. Not allowed, chunk is abandoned.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 1000).empty());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
    });
	}

	SECTION("retransmits when send buffer is full on T3-rtx expiry")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		constexpr size_t Cwnd{ 1200 };

		retransmissionQueue.SetCwnd(Cwnd);

		REQUIRE(retransmissionQueue.GetCwnd() == Cwnd);
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);

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

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1500) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
		REQUIRE(
		  retransmissionQueue.GetUnackedPacketBytes() ==
		  payload.size() + RTC::SCTP::DataChunk::DataChunkHeaderLength);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 1);

		// Will force chunks to be retransmitted.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
    });

		// After T3 expiry in-flight counters are cleared.
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);
		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1500) == std::vector<uint32_t>{ 10 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
		REQUIRE(
		  retransmissionQueue.GetUnackedPacketBytes() ==
		  payload.size() + RTC::SCTP::DataChunk::DataChunkHeaderLength);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 1);
	}

	SECTION("produces valid FORWARD-TSN")
	{
		// Three middle fragments (no "E"), same message (outgoingMessageId=42,
		// ssn=42). `Discard()` returns true, placeholder TSN 13 created.
		// FORWARD-TSN newCumulativeTsn=13, skippedStreams={ (streamId=1, ssn=42) }.

		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Ack TSN 10, but the remaining are lost.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		// T3 expiry: TSN 11, 12 abandoned. `Discard()` returns true, placeholder TSN 13.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ true);

		retransmissionQueue.HandleT3RtxTimerExpiry();

		// NOTE: TSN 13 represents the placeholder end fragment.
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 13, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		const std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };
		const auto* forwardTsnChunk = retransmissionQueue.AddForwardTsn(packet.get());

		REQUIRE(forwardTsnChunk);
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 13);
		REQUIRE(
		  forwardTsnChunk->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { 1, 42 }
    });
	}

	SECTION("produces valid FORWARD-TSN when fully sent")
	{
		// Three fragments "B"/""/""E" (message fully sent). `Discard()` returns
		// false, no placeholder. FORWARD-TSN newCumulativeTsn=12.

		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Ack TSN 10, but the remaining are lost.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		// T3 expiry: TSN 11, 12 abandoned. `Discard()` returns false, no placeholder.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		const std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };
		const auto* forwardTsnChunk = retransmissionQueue.AddForwardTsn(packet.get());

		REQUIRE(forwardTsnChunk);
		REQUIRE(forwardTsnChunk->GetNewCumulativeTsn() == 12);
		REQUIRE(
		  forwardTsnChunk->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { 1, 42 }
    });
	}

	SECTION("produces valid I-FORWARD-TSN")
	{
		auto retransmissionQueue = createRetransmissionQueue(
		  /*supportsPartialReliability*/ true, /*useMessageInterleaving*/ true);

		// Stream 1, ordered, outgoingMessageId=42, mid=42, "B".
		sendQueue
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 42, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 2, unordered, outgoingMessageId=43, mid=42, "B".
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(2, 0, 42, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, true);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(43, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 3, ordered, outgoingMessageId=44, mid=42, "B".
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(3, 0, 42, 0, 53, { 0x09, 0x0a, 0x0b, 0x0c }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(44, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  // Stream 4, ordered, outgoingMessageId=45, mid=42, "B".
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

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10, 11, 12, 13 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// TSN 13 is acked via gap block; TSN 10-12 are nacked.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 4, 4 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		// I-FORWARD-TSN: newCumulativeTsn=12 (can't go past ACKED TSN 13).
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sctpOptions.mtu) };

		const auto* iForwardTsnChunk1 = retransmissionQueue.AddIForwardTsn(packet.get());

		REQUIRE(iForwardTsnChunk1);
		REQUIRE(iForwardTsnChunk1->GetNewCumulativeTsn() == 12);
		REQUIRE(
		  iForwardTsnChunk1->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { false, 1, 42 },
		    { false, 3, 42 },
		    { true,  2, 42 },
    });

		// When TSN 13 is acked, the placeholder end fragments must be skipped too.
		// A receiver is more likely to ack TSN 13, but do it incrementally.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(12, Arwnd).get());

		sendQueue.ExpectDiscardCalledTimes(0);

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);
		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(13, Arwnd).get());

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 15, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 16, RTC::SCTP::OutstandingData::State::ABANDONED },
    });

		packet.reset(RTC::SCTP::Packet::Factory(sctpCommon::FactoryBuffer, sctpOptions.mtu));

		const auto* iForwardTsnChunk2 = retransmissionQueue.AddIForwardTsn(packet.get());

		REQUIRE(iForwardTsnChunk2);
		REQUIRE(iForwardTsnChunk2->GetNewCumulativeTsn() == 16);
		REQUIRE(
		  iForwardTsnChunk2->GetSkippedStreams() ==
		  std::vector<RTC::SCTP::AnyForwardTsnChunk::SkippedStream>{
		    { false, 1, 42 },
		    { false, 3, 42 },
		    { true,  2, 42 },
    });
	}

	SECTION("measure RTT")
	{
		auto retransmissionQueue = createRetransmissionQueue(
		  /*supportsPartialReliability*/ true, /*useMessageInterleaving*/ true);

		sendQueue.WillProduceOnce(createDataToSend(0, /*maxRetranmissions*/ 0))
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10 });

		constexpr uint64_t DurationMs{ 123 };

		nowMs += DurationMs;

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(retransmissionQueueListener.lastRttMs == DurationMs);
	}

	SECTION("validate cumulative TSN at rest")
	{
		// Nothing outstanding. TSN 8 is below lastCumulativeTsnAck(9) -> rejected.
		// TSN 9 equals lastCumulativeTsnAck(9) -> accepted (no-op).
		// TSN 10 is above highestOutstandingTsn(9) -> rejected.

		auto retransmissionQueue = createRetransmissionQueue();

		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(8, Arwnd).get()) == false);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(9, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get()) == false);
	}

	SECTION("validate cumulative TSN ack on inflight data")
	{
		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(8, Arwnd).get()) == false);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(9, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(11, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(12, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(13, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(14, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(15, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(16, Arwnd).get()) == true);
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(17, Arwnd).get()) == true);
		// TSN 18 has never been sent -> rejected.
		REQUIRE(
		  retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(18, Arwnd).get()) == false);
	}

	SECTION("handle gap-ack-blocks matching no inflight data")
	{
		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		// Ack 9, 20-25. This is an invalid SACK chunk, but should still be handled.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 11, 16 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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

	SECTION("handle invalid gap-ack-blocks")
	{
		// Nothing outstanding. Gap blocks referencing non-existent TSNs are
		// rejected.

		auto retransmissionQueue = createRetransmissionQueue();

		// cumTsn=9 (no change), gap {3,4} -> TSN 12-13, both beyond
		// highestOutstandingTsn(9) -> rejected. State unchanged.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 3, 4 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9, RTC::SCTP::OutstandingData::State::ACKED },
    });
	}

	SECTION("gap-ack-blocks do not move cumulative TSN ack")
	{
		// cumTsn=9, gap {1,5} acks TSN 10-14 via gap blocks. The cumulative TSN
		// ack point itself must NOT advance, gap acks are renegable.

		auto retransmissionQueue = createRetransmissionQueue();

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

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17 });

		// Ack 9, 10-14. This is actually an invalid ACK as the first gap can't be
		// adjacent to the cum-tsn-ack, but it's not strictly forbidden. However,
		// the cum-tsn-ack should not move, as the gap-ack-blocks are just advisory.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 1, 5 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 15, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 16, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 17, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
	}

	SECTION("stays within available size")
	{
		// With `GetChunksToSend(nowMs, 1188-12=1176)`, the first `Produce()` receives
		// 1176 - DataChunkHeaderLength bytes, the second receives the remainder.

		auto retransmissionQueue = createRetransmissionQueue();
		constexpr size_t AvailableBytes{ 1188 - 12 }; // 1176

		bool sizeCheck1Ok{ false };
		bool sizeCheck2Ok{ false };

		sendQueue
		  .WillProduceOnce(
		    [&sizeCheck1Ok](uint64_t /*nowMs*/, size_t maxLength)
		    {
			    sizeCheck1Ok = (maxLength == AvailableBytes - RTC::SCTP::DataChunk::DataChunkHeaderLength);

			    std::vector<uint8_t> payload(183, 0x00);

			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, std::move(payload), true, true, false));
		    })
		  .WillProduceOnce(
		    [&sizeCheck2Ok](uint64_t /*nowMs*/, size_t maxLength)
		    {
			    sizeCheck2Ok = (maxLength == 976 - RTC::SCTP::DataChunk::DataChunkHeaderLength);

			    std::vector<uint8_t> payload(957, 0x00);

			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      1, RTC::SCTP::UserData(1, 0, 0, 0, 53, std::move(payload), true, true, false));
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue, AvailableBytes) == std::vector<uint32_t>{ 10, 11 });
		REQUIRE(sizeCheck1Ok == true);
		REQUIRE(sizeCheck2Ok == true);
	}

	SECTION("accounts nacked abandoned chunks as not outstanding")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		const size_t chunkSerializedLength = RTC::SCTP::DataChunk::DataChunkHeaderLength + 4;

		REQUIRE(chunkSerializedLength == 16 + 4);

		// Three middle fragments of the same message, maxRetransmissions=0.
		sendQueue
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x05, 0x06, 0x07, 0x08 }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceOnce(
		    [](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x09, 0x0a, 0x0b, 0x0c }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.maxRetransmissions = 0;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == chunkSerializedLength * 3);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 3);

		// Mark the message as lost.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleT3RtxTimerExpiry();

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED },
    });
		// Abandoned chunks are not counted as outstanding.
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);

		// Acking abandoned chunks one by one changes nothing in the counters.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(11, Arwnd).get());

		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(12, Arwnd).get());

		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == 0);
		REQUIRE(retransmissionQueue.GetUnackedItems() == 0);
	}

	SECTION("expire from send queue when partially sent")
	{
		// Two fragments on stream 17, outgoingMessageId=42. First is produced and
		// goes in flight. After nowMs advances past `expiresAtMs`, the second is
		// produced but expired on Insert() -> first also abandoned, `Discard()`
		// called (returns true -> placeholder TSN 12).

		auto retransmissionQueue = createRetransmissionQueue();

		const uint64_t expiresAtMs = nowMs + 10;

		sendQueue
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(17, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(17, 0, 0, 0, 53, { 0x05, 0x06, 0x07, 0x08 }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		// First `GetChunksToSend()` produces TSN 10 (nowMs < expiresAtMs).
		REQUIRE(getSentPacketTSNs(retransmissionQueue, 24) == std::vector<uint32_t>{ 10 });

		// Advance past expiry.
		nowMs += 100;

		// `Discard()` called for TSN 11 (unsent tail) -> returns true -> placeholder
		// TSN 12.
		sendQueue.WillDiscardOnce(17, 42, /*returnValue*/ true);

		// Second `GetChunksToSend()` produces TSN 11 but now > expiresAtMs ->
		// abandoned on `Insert()`, TSN 10 also abandoned, placeholder TSN 12
		// created.
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 24).empty());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     }, // Initial TSN.
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED }, // Produced and in-flight.
		    { 11, RTC::SCTP::OutstandingData::State::ABANDONED }, // Produced and expired.
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED }, // Placeholder end.
    });
	}

	SECTION("expire correct message from send queue")
	{
		// Three messages on stream 1. Messages 42 (mid=0) and 43 (mid=1) are
		// complete single-fragment messages. Message 44 (mid=0, stream reset)
		// has a beginning fragment produced before expiry and a middle fragment
		// produced after expiry -> message 44 gets abandoned, messages 42 and 43
		// remain IN_FLIGHT.

		auto retransmissionQueue = createRetransmissionQueue();

		const uint64_t expiresAtMs = nowMs + 10;

		// outgoingMessageId=42, mid=0, "BE" — complete message.
		sendQueue
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, true, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(42, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  // outgoingMessageId=43, mid=1, "BE" — complete message.
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 1, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, true, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(43, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  // outgoingMessageId=44, mid=0 (stream reset), "B" — beginning only.
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 }, true, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(44, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  // outgoingMessageId=44, mid=0, middle fragment (produced after expiry).
		  .WillProduceOnce(
		    [expiresAtMs](uint64_t /*nowMs*/, size_t /*maxLength*/)
		    {
			    RTC::SCTP::UserData data(1, 0, 0, 0, 53, { 0x05, 0x06, 0x07, 0x08 }, false, false, false);
			    RTC::SCTP::SendQueueInterface::DataToSend dataToSend(44, std::move(data));

			    dataToSend.expiresAtMs = expiresAtMs;

			    return dataToSend;
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		std::vector<std::pair<uint32_t, RTC::SCTP::UserData>> expectedChunksToSend;

		// TSN 10, msgId=42.
		expectedChunksToSend.emplace_back(
		  10,
		  RTC::SCTP::UserData{
		    1, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 },
             true, true, false
    });

		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 24) == expectedChunksToSend);

		// TSN 11, msgId=43.
		expectedChunksToSend.clear();
		expectedChunksToSend.emplace_back(
		  11,
		  RTC::SCTP::UserData{
		    1, 0, 1, 0, 53, { 0x01, 0x02, 0x03, 0x04 },
             true, true, false
    });

		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 24) == expectedChunksToSend);

		// TSN 12, msgId=44 "B"
		expectedChunksToSend.clear();
		expectedChunksToSend.emplace_back(
		  12,
		  RTC::SCTP::UserData{
		    1, 0, 0, 0, 53, { 0x01, 0x02, 0x03, 0x04 },
             true, false, false
    });

		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 24) == expectedChunksToSend);

		// Advance past expiry.
		nowMs += 100;

		// `Discard()` called for message 44 (unsent middle fragment), returns true
		// -> placeholder TSN 14 created.
		sendQueue.WillDiscardOnce(1, 44, /*returnValue*/ true);

		// Fourth call produces TSN 13 (middle of message 44) but it's now expired
		// -> TSN 12 and 13 abandoned, placeholder TSN 14 created.
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 24).empty());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     }, // Initial TSN.
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT }, // msgId=42, BE.
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT }, // msgId=43, BE.
		    { 12, RTC::SCTP::OutstandingData::State::ABANDONED }, // msgId=44, B.
		    { 13, RTC::SCTP::OutstandingData::State::ABANDONED }, // msgId=44, produced and expired.
		    { 14, RTC::SCTP::OutstandingData::State::ABANDONED }, // Placeholder end.
    });
	}

	SECTION("limits retransmissions only when nacked three times")
	{
		// A chunk with maxRetransmissions=0 is NOT abandoned immediately when
		// nacked — it takes exactly three nacks like any other chunk, and is
		// abandoned on the third (not retransmitted, since maxRetransmissions=0).

		auto retransmissionQueue = createRetransmissionQueue();

		// TSN 10: maxRetransmissions=0.
		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 0))
		  .WillProduceOnce(createDataToSend(0)) // TSN 11.
		  .WillProduceOnce(createDataToSend(1)) // TSN 12.
		  .WillProduceOnce(createDataToSend(2)) // TSN 13.
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10, 11, 12, 13 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		// `Discard()` must NOT be called for the first two nacks.
		sendQueue.ExpectDiscardCalledTimes(0);

		// First nack for TSN 10.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 2, 2 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::NACKED    },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		// Second nack for TSN 10.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 2, 3 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::NACKED    },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		// Third nack -> TSN 10 abandoned (maxRetransmissions=0 means 0
		// retransmits).
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 2, 4 },
    })
		    .get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);
	}

	SECTION("abandons rtx limit 2 when nacked nine times")
	{
		// maxRetransmits=2 for TSN 10: first 3 nacks -> fast-retransmit #1;
		// next 3 nacks -> regular retransmit #2; next 3 nacks -> abandoned.

		auto retransmissionQueue = createRetransmissionQueue();

		// TSN 10: maxRetransmissions=2.
		sendQueue.WillProduceOnce(createDataToSend(42, /*maxRetransmissions*/ 2))
		  .WillProduceOnce(createDataToSend(0)) // TSN 11.
		  .WillProduceOnce(createDataToSend(1)) // TSN 12.
		  .WillProduceOnce(createDataToSend(2)) // TSN 13.
		  .WillProduceOnce(createDataToSend(3)) // TSN 14.
		  .WillProduceOnce(createDataToSend(4)) // TSN 15.
		  .WillProduceOnce(createDataToSend(5)) // TSN 16.
		  .WillProduceOnce(createDataToSend(6)) // TSN 17.
		  .WillProduceOnce(createDataToSend(7)) // TSN 18.
		  .WillProduceOnce(createDataToSend(8)) // TSN 19.
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue) ==
		  std::vector<uint32_t>{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
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
		    { 18, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 19, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// No `Discard()` calls during nack rounds 1 and 2.
		sendQueue.ExpectDiscardCalledTimes(0);

		// Ack TSN 11-13 — three nacks for TSN 10 -> TO_BE_RETRANSMITTED.
		for (uint32_t tsn{ 11 }; tsn <= 13; ++tsn)
		{
			retransmissionQueue.HandleReceivedSackChunk(
			  nowMs,
			  createSackChunk(
			    9,
			    Arwnd,
			    {
			      { 2, static_cast<uint16_t>(tsn - 9) }
      })
			    .get());
		}

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 14, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 15, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 16, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 17, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 18, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 19, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
    });

		// Fast retransmit #1.
		REQUIRE(getTSNsForFastRetransmit(retransmissionQueue) == std::vector<uint32_t>{ 10 });

		// Ack TSN 14-16 — three more nacks -> retransmit #2 (TO_BE_RETRANSMITTED).
		for (uint32_t tsn{ 14 }; tsn <= 16; ++tsn)
		{
			retransmissionQueue.HandleReceivedSackChunk(
			  nowMs,
			  createSackChunk(
			    9,
			    Arwnd,
			    {
			      { 2, static_cast<uint16_t>(tsn - 9) }
      })
			    .get());
		}

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED               },
		    { 10, RTC::SCTP::OutstandingData::State::TO_BE_RETRANSMITTED },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 16, RTC::SCTP::OutstandingData::State::ACKED               },
		    { 17, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 18, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
		    { 19, RTC::SCTP::OutstandingData::State::IN_FLIGHT           },
    });

		// Regular retransmit #2.
		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1000) == std::vector<uint32_t>{ 10 });

		// Ack TSN 17-18 — two more nacks (TSN 10 is now in-flight again after retransmit).
		for (uint32_t tsn{ 17 }; tsn <= 18; ++tsn)
		{
			retransmissionQueue.HandleReceivedSackChunk(
			  nowMs,
			  createSackChunk(
			    9,
			    Arwnd,
			    {
			      { 2, static_cast<uint16_t>(tsn - 9) }
      })
			    .get());
		}

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::NACKED    },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 16, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 18, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 19, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == false);

		REQUIRE_VERIFICATION_RESULT(sendQueue.VerifyExpectations());

		// Ack TSN 19 — third nack; numRetransmissions(2) == maxRetransmissions(2)
		// -> ABANDON.
		sendQueue.WillDiscardOnce(1, 42, /*returnValue*/ false);

		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    Arwnd,
		    {
		      { 2, 10 }
    })
		    .get());

		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, 1000).empty());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::ABANDONED },
		    { 11, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 12, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 13, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 14, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 15, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 16, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 17, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 18, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 19, RTC::SCTP::OutstandingData::State::ACKED     },
    });

		REQUIRE(retransmissionQueue.ShouldSendForwardTsn(nowMs) == true);
	}

	SECTION("cwnd recovers when acking")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		constexpr size_t Cwnd{ 1200 };

		retransmissionQueue.SetCwnd(Cwnd);

		REQUIRE(retransmissionQueue.GetCwnd() == Cwnd);

		const std::vector<uint8_t> payload(1000, 0x00);
		const size_t chunkSerializedLength = RTC::SCTP::DataChunk::DataChunkHeaderLength + payload.size();

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

		REQUIRE(getSentPacketTSNs(retransmissionQueue, 1500) == std::vector<uint32_t>{ 10 });
		REQUIRE(retransmissionQueue.GetUnackedPacketBytes() == chunkSerializedLength);

		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, Arwnd).get());

		REQUIRE(retransmissionQueue.GetCwnd() == Cwnd + chunkSerializedLength);
	}

	SECTION("can always send one packet")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		const size_t mtu{ Utils::Byte::PadDownTo4Bytes(Mtu) }; // 1188.
		const std::vector<uint8_t> payload(mtu - 100, 0x00);   // 1088 bytes.

		sendQueue
		  .WillProduceOnce(
		    [&payload](uint64_t, size_t)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, true, false, false));
		    })
		  .WillProduceOnce(
		    [&payload](uint64_t, size_t)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, false, false, false));
		    })
		  .WillProduceOnce(
		    [&payload](uint64_t, size_t)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, false, false, false));
		    })
		  .WillProduceOnce(
		    [&payload](uint64_t, size_t)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, false, false, false));
		    })
		  .WillProduceOnce(
		    [&payload](uint64_t, size_t)
		    {
			    return RTC::SCTP::SendQueueInterface::DataToSend(
			      0, RTC::SCTP::UserData(1, 0, 0, 0, 53, payload, false, true, false));
		    })
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		// Produce all 5 chunks (TSN 10-14) in one call.
		REQUIRE(
		  getSentPacketTSNs(retransmissionQueue, 5 * mtu) == std::vector<uint32_t>{ 10, 11, 12, 13, 14 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 13, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 14, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		// Ack 12, and report an empty receiver window (the peer obviously has a
		// tiny receive window).
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    /*aRwnd*/ 0,
		    {
		      { 3, 3 }
    })
		    .get());

		// Force TSN 10 to be retransmitted.
		retransmissionQueue.HandleT3RtxTimerExpiry();

		// Even with rwnd=0, one packet can be sent (no in-flight data after NackAll).
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu) == std::vector<uint32_t>{ 10 });

		// But not a second one — TSN 10 is now in-flight again.
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu).empty());

		// Still rwnd=0, TSN 10 in-flight.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    9,
		    /*aRwnd=*/0,
		    {
		      { 3, 3 }
    })
		    .get());

		// There is in-flight data, so new data should not be allowed to be send since
		// the receiver window is full.
		REQUIRE(retransmissionQueue.GetChunksToSend(nowMs, mtu).empty());

		// Ack TSN 10 (no more in-flight data), still rwnd=0.
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs,
		  createSackChunk(
		    10,
		    /*aRwnd=*/0,
		    {
		      { 2, 2 }
    })
		    .get());

		// TSN 11 can be sent since there is no in-flight data.
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu) == std::vector<uint32_t>{ 11 });

		// But not a second one.
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu).empty());

		// Ack and recover the receiver window
		retransmissionQueue.HandleReceivedSackChunk(
		  nowMs, createSackChunk(12, static_cast<uint32_t>(5 * mtu)).get());

		// Remaining TO_BE_RETRANSMITTED chunks can now be sent.
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu) == std::vector<uint32_t>{ 13 });
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu) == std::vector<uint32_t>{ 14 });
		REQUIRE(getSentPacketTSNs(retransmissionQueue, mtu).empty());
	}

	SECTION("updates rwnd from SACK and unacked payload bytes")
	{
		auto retransmissionQueue = createRetransmissionQueue();

		REQUIRE(retransmissionQueue.GetRwnd() == Arwnd);

		// Payload is 4 bytes (padded to 4).
		constexpr size_t PayloadSize{ 4 };

		sendQueue
		  .WillProduceOnce(createDataToSend(0)) // TSN 10.
		  .WillProduceOnce(createDataToSend(1)) // TSN 11.
		  .WillProduceOnce(createDataToSend(2)) // TSN 12.
		  .WillProduceRepeatedly(
		    [](uint64_t, size_t)
		    {
			    return std::nullopt;
		    });

		REQUIRE(getSentPacketTSNs(retransmissionQueue) == std::vector<uint32_t>{ 10, 11, 12 });
		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 9,  RTC::SCTP::OutstandingData::State::ACKED     },
		    { 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.GetRwnd() == Arwnd - (PayloadSize * 3));

		// Ack TSN 10, new aRwnd=1000.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(10, 1000).get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 10, RTC::SCTP::OutstandingData::State::ACKED     },
		    { 11, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		    { 12, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
    });

		REQUIRE(retransmissionQueue.GetRwnd() == 1000 - (PayloadSize * 2));

		// Ack everything, new aRwnd=2000.
		retransmissionQueue.HandleReceivedSackChunk(nowMs, createSackChunk(12, 2000).get());

		REQUIRE(
		  retransmissionQueue.GetChunkStatesForTesting() ==
		  std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>>{
		    { 12, RTC::SCTP::OutstandingData::State::ACKED },
    });

		REQUIRE(retransmissionQueue.GetRwnd() == 2000);
	}
}
