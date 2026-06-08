#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "RTC/SCTP/association/AssociationListenerDeferrer.hpp"
#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/rx/DataTracker.hpp"
#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "RTC/SCTP/tx/RoundRobinSendQueue.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/association/MockTransmissionControlBlockContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
	constexpr uint32_t Arwnd{ 131072 };
	constexpr uint32_t LocalInitialTsn{ 0 };
	constexpr uint32_t RemoteInitialTsn{ 0 };
	constexpr uint64_t InitialNowMs{ 10000 };
	constexpr uint64_t RtoMs{ 250 };

	/**
	 * A RTC::SCTP::StreamResetHandler under test, together with all the (real)
	 * components it depends on, its simulated clock and listener.
	 */
	class TestStreamResetHandler
	{
	public:
		TestStreamResetHandler()
		  // NOTE: The order in which these members are initialized is **critical**.
		  : shared(/*getTimeMs*/
		           [this]()
		           {
			           return this->nowMs;
		           }),
		    tcbContext(this->associationListener, this->sctpOptions),
		    delayedAckTimer(this->shared.CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener         = std::addressof(this->backoffTimerListener),
		        .label            = "mock-sctp-delayed-ack",
		        .baseTimeoutMs    = this->sctpOptions.initialRtoMs,
		        .backoffAlgorithm = BackoffTimerHandleInterface::BackoffAlgorithm::FIXED })),
		    t3RtxTimer(this->shared.CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener         = std::addressof(this->backoffTimerListener),
		        .label            = "mock-sctp-t3-rtx",
		        .baseTimeoutMs    = this->sctpOptions.initialRtoMs,
		        .backoffAlgorithm = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL })),
		    sendQueue(
		      this->associationListener,
		      this->sctpOptions.mtu,
		      this->sctpOptions.defaultStreamPriority,
		      this->sctpOptions.totalBufferedAmountLowThreshold),
		    dataTracker(this->delayedAckTimer.get(), RemoteInitialTsn),
		    reassemblyQueue(this->sctpOptions.maxReceiverWindowBufferSize),
		    retransmissionQueue(
		      std::addressof(this->retransmissionQueueListener),
		      this->associationListener,
		      LocalInitialTsn,
		      Arwnd,
		      this->sendQueue,
		      this->t3RtxTimer.get(),
		      this->sctpOptions,
		      /*supportsPartialReliability*/ true,
		      /*useMessageInterleaving*/ false),
		    streamResetHandler(
		      this->associationListenerDeferrer,
		      std::addressof(this->shared),
		      std::addressof(this->tcbContext),
		      std::addressof(this->dataTracker),
		      std::addressof(this->reassemblyQueue),
		      std::addressof(this->retransmissionQueue))
		{
			this->tcbContext.WillGetCurrentRtoMsOnce(
			  []()
			  {
				  return RtoMs;
			  });
		}

	public:
		/**
		 * Advances the simulated clock by `incrementMs`.
		 */
		void AdvanceTimeMs(uint64_t incrementMs)
		{
			this->nowMs += incrementMs;
		}

		/**
		 * Handles a received RE-CONFIG chunk within a deferrer scope, just like
		 * association does before dispatching to the handler.
		 */
		void HandleReceivedReConfigChunk(const RTC::SCTP::ReConfigChunk* receivedReConfigChunk)
		{
			const RTC::SCTP::AssociationListenerDeferrer::ScopedDeferrer deferrer(
			  this->associationListenerDeferrer);

			this->streamResetHandler.HandleReceivedReConfigChunk(receivedReConfigChunk);
		}

	private:
		/**
		 * A no-op BackoffTimerHandle listener for the timers owned directly by this
		 * fixture (the ones owned by the handler are driven by the handler itself).
		 */
		class BackoffTimerListener : public BackoffTimerHandleInterface::Listener
		{
		public:
			void OnBackoffTimer(
			  BackoffTimerHandleInterface* /*backoffTimer*/, uint64_t& /*baseTimeoutMs*/, bool& /*stop*/) override
			{
			}
		};

		/**
		 * A no-op RTC::SCTP::RetransmissionQueue listener.
		 */
		class RetransmissionQueueListener : public RTC::SCTP::RetransmissionQueue::Listener
		{
		public:
			void OnRetransmissionQueueNewRttMs(uint64_t /*rttMs*/) override
			{
			}

			void OnRetransmissionQueueClearRetransmissionCounter() override
			{
			}
		};

		// NOTE: Public members for testing.
	public:
		uint64_t nowMs{ InitialNowMs };
		RTC::SCTP::SctpOptions sctpOptions;
		BackoffTimerListener backoffTimerListener;
		RetransmissionQueueListener retransmissionQueueListener;
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::AssociationListenerDeferrer associationListenerDeferrer{ std::addressof(
			this->associationListener) };
		mocks::MockShared shared;
		mocks::RTC::SCTP::MockTransmissionControlBlockContext tcbContext;
		const std::unique_ptr<BackoffTimerHandleInterface> delayedAckTimer;
		const std::unique_ptr<BackoffTimerHandleInterface> t3RtxTimer;
		RTC::SCTP::RoundRobinSendQueue sendQueue;
		RTC::SCTP::DataTracker dataTracker;
		RTC::SCTP::ReassemblyQueue reassemblyQueue;
		RTC::SCTP::RetransmissionQueue retransmissionQueue;
		RTC::SCTP::StreamResetHandler streamResetHandler;
	};
} // namespace

SCENARIO("SCTP RTC::SCTP::StreamResetHandler", "[sctp][streamresethandler]")
{
	SECTION("a chunk with no parameters returns an error")
	{
		TestStreamResetHandler test;

		std::vector<uint8_t> buffer(test.sctpOptions.mtu);

		const std::unique_ptr<RTC::SCTP::ReConfigChunk> reConfigChunk{ RTC::SCTP::ReConfigChunk::Factory(
			buffer.data(), buffer.size()) };

		test.HandleReceivedReConfigChunk(reConfigChunk.get());

		REQUIRE(test.associationListener.HasErrored() == true);
		REQUIRE(test.associationListener.HasSentPackets() == false);
	}

	SECTION("a chunk with invalid parameters returns an error")
	{
		TestStreamResetHandler test;

		std::vector<uint8_t> buffer(test.sctpOptions.mtu);

		const std::unique_ptr<RTC::SCTP::ReConfigChunk> reConfigChunk{ RTC::SCTP::ReConfigChunk::Factory(
			buffer.data(), buffer.size()) };

		// Two RTC::SCTP::OutgoingSsnResetRequestParameter in a RE-CONFIG is not valid.
		for (const uint32_t reqSeqNbr : { 1u, 2u })
		{
			auto* parameter =
			  reConfigChunk->BuildParameterInPlace<RTC::SCTP::OutgoingSsnResetRequestParameter>();

			parameter->SetReconfigurationRequestSequenceNumber(reqSeqNbr);
			parameter->SetReconfigurationResponseSequenceNumber(10);
			parameter->SetSenderLastAssignedTsn(RemoteInitialTsn);
			parameter->AddStreamId(static_cast<uint16_t>(reqSeqNbr));
			parameter->Consolidate();
		}

		test.HandleReceivedReConfigChunk(reConfigChunk.get());

		REQUIRE(test.associationListener.HasErrored() == true);
		REQUIRE(test.associationListener.HasSentPackets() == false);
	}

	SECTION("sends an outgoing request directly")
	{
		TestStreamResetHandler test;

		const std::vector<uint16_t> streamIds{ 42 };

		test.streamResetHandler.ResetStreams(streamIds);

		REQUIRE(test.streamResetHandler.ShouldSendStreamResetRequest() == true);

		const auto packet = test.tcbContext.CreatePacket();

		test.streamResetHandler.AddStreamResetRequest(packet.get());

		const auto* reConfigChunk = packet->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>();

		REQUIRE(reConfigChunk);

		const auto* parameter =
		  reConfigChunk->GetFirstParameterOfType<RTC::SCTP::OutgoingSsnResetRequestParameter>();

		REQUIRE(parameter);
		REQUIRE(parameter->GetStreamIds() == std::vector<uint16_t>{ 42 });
	}

	SECTION("resets multiple streams in one request")
	{
		TestStreamResetHandler test;

		test.streamResetHandler.ResetStreams(std::vector<uint16_t>{ 42 });
		test.streamResetHandler.ResetStreams(std::vector<uint16_t>{ 43, 44, 41 });
		test.streamResetHandler.ResetStreams(std::vector<uint16_t>{ 42, 40 });

		REQUIRE(test.streamResetHandler.ShouldSendStreamResetRequest() == true);

		const auto packet = test.tcbContext.CreatePacket();

		test.streamResetHandler.AddStreamResetRequest(packet.get());

		const auto* reConfigChunk = packet->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>();

		REQUIRE(reConfigChunk);

		const auto* parameter =
		  reConfigChunk->GetFirstParameterOfType<RTC::SCTP::OutgoingSsnResetRequestParameter>();

		REQUIRE(parameter);

		auto streamIds = parameter->GetStreamIds();

		std::ranges::sort(streamIds);

		REQUIRE(streamIds == std::vector<uint16_t>{ 40, 41, 42, 43, 44 });
	}

	SECTION("commits the reset on a positive response")
	{
		TestStreamResetHandler test;

		test.streamResetHandler.ResetStreams(std::vector<uint16_t>{ 42 });

		REQUIRE(test.streamResetHandler.ShouldSendStreamResetRequest() == true);

		const auto packet = test.tcbContext.CreatePacket();

		test.streamResetHandler.AddStreamResetRequest(packet.get());

		const auto* parameter =
		  packet->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>()
		    ->GetFirstParameterOfType<RTC::SCTP::OutgoingSsnResetRequestParameter>();

		REQUIRE(parameter);

		const uint32_t reqSeqNbr = parameter->GetReconfigurationRequestSequenceNumber();

		// Build a RE-CONFIG response with a successful result.
		std::vector<uint8_t> responseBuffer(test.sctpOptions.mtu);
		const std::unique_ptr<RTC::SCTP::ReConfigChunk> responseChunk{ RTC::SCTP::ReConfigChunk::Factory(
			responseBuffer.data(), responseBuffer.size()) };

		auto* response =
		  responseChunk->BuildParameterInPlace<RTC::SCTP::ReconfigurationResponseParameter>();

		response->SetReconfigurationResponseSequenceNumber(reqSeqNbr);
		response->SetResult(RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED);
		response->Consolidate();

		test.HandleReceivedReConfigChunk(responseChunk.get());

		REQUIRE(test.associationListener.HasStreamsResetPerformedForStreamId(42) == true);
	}

	SECTION("rolls back the reset on an error response")
	{
		TestStreamResetHandler test;

		test.streamResetHandler.ResetStreams(std::vector<uint16_t>{ 42 });

		REQUIRE(test.streamResetHandler.ShouldSendStreamResetRequest() == true);

		const auto packet = test.tcbContext.CreatePacket();

		test.streamResetHandler.AddStreamResetRequest(packet.get());

		const auto* parameter =
		  packet->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>()
		    ->GetFirstParameterOfType<RTC::SCTP::OutgoingSsnResetRequestParameter>();

		REQUIRE(parameter);

		const uint32_t reqSeqNbr = parameter->GetReconfigurationRequestSequenceNumber();

		// Build a RE-CONFIG response with an error result.
		std::vector<uint8_t> responseBuffer(test.sctpOptions.mtu);
		const std::unique_ptr<RTC::SCTP::ReConfigChunk> responseChunk{ RTC::SCTP::ReConfigChunk::Factory(
			responseBuffer.data(), responseBuffer.size()) };

		auto* response =
		  responseChunk->BuildParameterInPlace<RTC::SCTP::ReconfigurationResponseParameter>();

		response->SetReconfigurationResponseSequenceNumber(reqSeqNbr);
		response->SetResult(
		  RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER);
		response->Consolidate();

		test.HandleReceivedReConfigChunk(responseChunk.get());

		REQUIRE(test.associationListener.HasStreamsResetFailedForStreamId(42) == true);
	}

	SECTION("an incoming stream reset request returns nothing performed")
	{
		TestStreamResetHandler test;

		std::vector<uint8_t> buffer(test.sctpOptions.mtu);
		const std::unique_ptr<RTC::SCTP::ReConfigChunk> reConfigChunk{ RTC::SCTP::ReConfigChunk::Factory(
			buffer.data(), buffer.size()) };

		auto* parameter =
		  reConfigChunk->BuildParameterInPlace<RTC::SCTP::IncomingSsnResetRequestParameter>();

		parameter->SetReconfigurationRequestSequenceNumber(RemoteInitialTsn);
		parameter->AddStreamId(1);
		parameter->Consolidate();

		test.HandleReceivedReConfigChunk(reConfigChunk.get());

		// A RE-CONFIG response is sent back.
		const auto sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		REQUIRE(!sentBuffer.empty());

		const std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);

		const auto* response = sentPacket->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>()
		                         ->GetFirstParameterOfType<RTC::SCTP::ReconfigurationResponseParameter>();

		REQUIRE(response);
		REQUIRE(
		  response->GetResult() ==
		  RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO);
	}
}
