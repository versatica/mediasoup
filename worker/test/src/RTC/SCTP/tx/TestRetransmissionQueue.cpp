// #include "common.hpp"
// #include "Utils.hpp"
// #include "mocks/include/MockShared.hpp"
// #include "mocks/include/RTC/SCTP/public/MockAssociationListener.hpp"
// #include "mocks/include/RTC/SCTP/tx/MockSendQueue.hpp"
// #include "RTC/SCTP/packet/Packet.hpp"
// #include "RTC/SCTP/packet/UserData.hpp"
// #include "RTC/SCTP/packet/chunks/DataChunk.hpp"
// #include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
// #include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
// #include "RTC/SCTP/packet/chunks/SackChunk.hpp"
// #include "RTC/SCTP/public/SctpOptions.hpp"
// #include "RTC/SCTP/sctpCommon.hpp"
// #include "RTC/SCTP/tx/RetransmissionQueue.hpp"
// #include <catch2/catch_test_macros.hpp>
// #include <vector>

// SCENARIO("SCTP RetransmissionQueue", "[sctp][retransmissionqueue]")
// {
// 	sctpCommon::ResetBuffers();

// 	class MockRetransmissionQueueListener : public RTC::SCTP::RetransmissionQueue::Listener
// 	{
// 	public:
// 		void OnRetransmissionQueueNewRttMs(uint64_t rttMs) override
// 		{
// 			this->lastRttMs = rttMs;
// 		}

// 		void OnRetransmissionQueueClearRetransmissionCounter() override
// 		{
// 			++this->clearRetransmissionCounterCalls;
// 		}

// 	public:
// 		uint64_t lastRttMs{ 0 };
// 		size_t clearRetransmissionCounterCalls{ 0 };
// 	};

// 	constexpr uint64_t NowMs{ 0 };
// 	constexpr uint32_t RemoteAdvertisedReceiverWindowCredit{ 100000 };
// 	constexpr uint64_t Mtu{ 1191 };
// 	constexpr uint32_t MessageId{ 42 };
// 	// InitialTsn is the first TSN that will be assigned. The TSN before it
// 	// (InitialTsn - 1) starts as ACKED in OutstandingData, matching dcsctp's
// 	// invariant that the initial state has the previous TSN already
// 	// cumulative-acked.
// 	constexpr uint32_t InitialTsn{ 10 };
// 	constexpr uint32_t PreviousTsn{ InitialTsn - 1 };

// 	const mocks::MockShared shared;

// 	RTC::SCTP::SctpOptions sctpOptions;

// 	sctpOptions.mtu = Mtu;

// 	MockRetransmissionQueueListener queueListener;
// 	mocks::RTC::SCTP::MockAssociationListener associationListener;

// 	/**
// 	 * Builds a SACK Chunk in the shared factory buffer.
// 	 * Valid until the next `buildSackChunk()` call (buffer is reused).
// 	 *
// 	 * @remarks
// 	 * - Maps to dcsctp's inline construction:
// 	 *   `SackChunk(TSN(...), RemoteAdvertisedReceiverWindowCredit, {gaps}, {})`
// 	 */
// 	auto buildSackChunk =
// 	  [](uint32_t cumTsn, uint32_t aRwnd, const std::vector<RTC::SCTP::SackChunk::GapAckBlock>& gabs
// = {})
// 	{
// 		auto* sackChunk =
// 		  RTC::SCTP::SackChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

// 		sackChunk->SetCumulativeTsnAck(cumTsn);
// 		sackChunk->SetAdvertisedReceiverWindowCredit(aRwnd);

// 		for (const auto& gab : gabs)
// 		{
// 			sackChunk->AddAckBlock(gab.start, gab.end);
// 		}

// 		return sackChunk;
// 	};

// 	/**
// 	 * Creates a 1-byte UserData fragment on stream 0.
// 	 */
// 	auto createUserData = [](bool beginning = true, bool ending = true)
// 	{
// 		return RTC::SCTP::UserData(1, 0, 0, 0, 53, { 0x00 }, beginning, ending, false);
// 	};

// 	auto* t3RtxTimer = shared.CreateBackoffTimer(
// 	  BackoffTimerHandleInterface::BackoffTimerHandleOptions{
// 	    // No `listener` given on purpose.
// 	    .label               = "mock-sctp-t3-rtx",
// 	    .baseTimeoutMs       = sctpOptions.initialRtoMs,
// 	    .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
// 	    .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
// 	    .maxRestarts         = std::nullopt });

// 	// TODO: SCTP: Need MockSendQueue.

// 	// RTC::SCTP::RetransmissionQueue queue(
// 	//   std::addressof(queueListener),
// 	//   associationListener,
// 	//   InitialTsn,
// 	//   RemoteAdvertisedReceiverWindowCredit,
// 	//   // TODO: SCTP: Need MockSendQueue.
// 	//   SendQueueInterface& sendQueue,
// 	//   t3RtxTimer,
// 	//   sctpOptions,
// 	//   /*supportsPartialReliability*/ true,
// 	//   /*useMessageInterleaving*/ false);

// 	// SECTION("initial acked previous TSN")
// 	// {
// 	// 	REQUIRE(queue.GetUnackedItems() == 0);
// 	// 	REQUIRE(queue.GetUnackedPacketBytes() == 0);
// 	// 	REQUIRE(queue.GetNextTsn() == InitialTsn);
// 	// 	REQUIRE(queue.ShouldSendForwardTsn(NowMs) == false);

// 	// 	const std::vector<std::pair<uint32_t, RTC::SCTP::OutstandingData::State>> expected = {
// 	// 		{ PreviousTsn, RTC::SCTP::OutstandingData::State::ACKED },
// 	// 	};

// 	// 	REQUIRE(queue.GetChunkStatesForTesting() == expected);
// 	// }

// 	// TODO: SCTP: A lot of tests.
// }
