#include "common.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/public/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
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

	constexpr uint32_t RemoteAdvertisedReceiverWindowCredit{ 100000 };
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
		  RemoteAdvertisedReceiverWindowCredit,
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

		const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expected = {
			{ PreviousTsn, RTC::SCTP::OutstandingData::State::ACKED },
		};

		REQUIRE(queue.GetChunkStatesForTesting() == expected);
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

		const std::vector<uint32_t> expectedTsns = { 10 };

		REQUIRE(getSentPacketTSNs(queue) == expectedTsns);

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 9,  RTC::SCTP::OutstandingData::State::ACKED     },
			{ 10, RTC::SCTP::OutstandingData::State::IN_FLIGHT },
		};

		REQUIRE(queue.GetChunkStatesForTesting() == expectedState);
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

		const std::vector<uint32_t> expectedTsns = { 10 };

		REQUIRE(getSentPacketTSNs(queue) == expectedTsns);

		const std::unique_ptr<RTC::SCTP::SackChunk> chunk{ RTC::SCTP::SackChunk::Factory(
			sctpCommon::FactoryBuffer, Mtu) };

		chunk->SetAdvertisedReceiverWindowCredit(RemoteAdvertisedReceiverWindowCredit);
		chunk->SetCumulativeTsnAck(10);

		queue.HandleReceivedSackChunk(nowMs, chunk.get());

		const std::vector<std::pair<uint32_t /*tsn*/, RTC::SCTP::OutstandingData::State>> expectedState = {
			{ 10, RTC::SCTP::OutstandingData::State::ACKED },
		};

		REQUIRE(queue.GetChunkStatesForTesting() == expectedState);
	}

	// TODO: SCTP: A lot of tests.
}
