#include "common.hpp"
#include "RTC/SCTP/association/AssociationListenerDeferrer.hpp"
#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include "test/include/catch2Macros.hpp"
#include "test/include/testHelpers.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/association/MockTransmissionControlBlockContext.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("SCTP HeartbeatHandler", "[sctp][heartbeathandler]")
{
	constexpr int64_t InitialNowUs{ 1000000 * 1000 };
	constexpr int64_t HeartbeatIntervalMs{ 30000 };

	class TestHeartbeatHandler
	{
	public:
		explicit TestHeartbeatHandler(int64_t heartbeatIntervalMs)
		  // NOTE: The order in which these members are initialized is **critical**.
		  : sctpOptions(
		      RTC::SCTP::SctpOptions{
		        .heartbeatIntervalMs         = heartbeatIntervalMs,
		        .heartbeatIntervalIncludeRtt = false,
		        .zeroChecksumAlternateErrorDetectionMethod =
		          RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE }),
		    tcbContext(this->associationListener, this->sctpOptions),
		    shared(/*getTimeUs*/
				       [this]() -> int64_t
				       {
			           return this->nowUs;
		           }),
		    heartbeatHandler(
		      this->associationListenerDeferrer,
		      this->sctpOptions,
		      std::addressof(this->shared),
		      std::addressof(this->tcbContext))
		{
			// Simulate that the SCTP assiciation is connected.
			this->tcbContext.SetAssociationEstablished(true);
		};

	public:
		/**
		 * @remarks
		 * - The increment is given in milliseconds since it comes from the SCTP
		 *   options and the timers, which work in milliseconds.
		 */
		void AdvanceNowMs(int64_t incrementMs)
		{
			this->nowUs += incrementMs * 1000;
		}

	private:
		int64_t nowUs{ InitialNowUs };

		// NOTE: Public members for testing.
	public:
		const RTC::SCTP::SctpOptions sctpOptions;
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		RTC::SCTP::AssociationListenerDeferrer associationListenerDeferrer{ std::addressof(
			this->associationListener) };
		mocks::RTC::SCTP::MockTransmissionControlBlockContext tcbContext;
		mocks::MockShared shared;
		RTC::SCTP::HeartbeatHandler heartbeatHandler;
	};

	SECTION("has running heartbeat interval timer")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		test.AdvanceNowMs(test.sctpOptions.heartbeatIntervalMs);

		auto* heartbeatIntervalTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(heartbeatIntervalTimer);
		REQUIRE(heartbeatIntervalTimer->EvaluateHasExpired() == true);
		REQUIRE(test.associationListener.HasSentPackets() == true);

		const std::vector<uint8_t> sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->GetChunksCount() == 1);

		const auto* sentHeartbeatRequestChunk =
		  sentPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();

		REQUIRE(sentHeartbeatRequestChunk);

		const auto* sentHeartbeatInfoParameter =
		  sentHeartbeatRequestChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(sentHeartbeatInfoParameter);
		REQUIRE(sentHeartbeatInfoParameter->HasInfo());
	}

	SECTION("replies to heartbeat requests")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		std::unique_ptr<RTC::SCTP::HeartbeatRequestChunk> receivedHeartbeatRequestChunk{
			RTC::SCTP::HeartbeatRequestChunk::Factory(sctpCommon::FactoryBuffer, test.sctpOptions.mtu)
		};

		auto* receivedHeartbeatInfoParameter =
		  receivedHeartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

		receivedHeartbeatInfoParameter->SetInfo(sctpCommon::DataBuffer, 10);
		receivedHeartbeatInfoParameter->Consolidate();

		test.heartbeatHandler.HandleReceivedHeartbeatRequestChunk(receivedHeartbeatRequestChunk.get());

		const std::vector<uint8_t> sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->GetChunksCount() == 1);

		const auto* sentHeartbeatAckChunk =
		  sentPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>();

		REQUIRE(sentHeartbeatAckChunk);

		const auto* sentHeartbeatInfoParameter =
		  sentHeartbeatAckChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(sentHeartbeatInfoParameter);
		REQUIRE(sentHeartbeatInfoParameter->HasInfo());
		REQUIRE(
		  helpers::areBuffersEqual(
		    sentHeartbeatInfoParameter->GetBuffer(),
		    sentHeartbeatInfoParameter->GetLength(),
		    receivedHeartbeatInfoParameter->GetBuffer(),
		    receivedHeartbeatInfoParameter->GetLength()) == true);
	}

	SECTION("doesn't reply to heartbeat requests too large to be echoed")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		const auto buildHeartbeatRequestChunk =
		  [](uint16_t infoLength) -> std::unique_ptr<RTC::SCTP::HeartbeatRequestChunk>
		{
			std::unique_ptr<RTC::SCTP::HeartbeatRequestChunk> heartbeatRequestChunk{
				RTC::SCTP::HeartbeatRequestChunk::Factory(
				  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
			};

			auto* const heartbeatInfoParameter =
			  heartbeatRequestChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

			heartbeatInfoParameter->SetInfo(sctpCommon::DataBuffer, infoLength);
			heartbeatInfoParameter->Consolidate();

			return heartbeatRequestChunk;
		};

		// The reply takes 12 bytes of common header, 4 of chunk header and the
		// echoed Heartbeat Info parameter, which is its 4 bytes header plus the
		// info padded to 4 bytes. With 1168 bytes of info that is 1188 bytes,
		// which fit in the 1191 bytes a packet can take.
		const auto fittingHeartbeatRequestChunk = buildHeartbeatRequestChunk(1168);

		test.heartbeatHandler.HandleReceivedHeartbeatRequestChunk(fittingHeartbeatRequestChunk.get());

		const std::vector<uint8_t> sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		REQUIRE(sentBuffer.size() == 1188);

		// With 1169 bytes of info the padded parameter grows by 4 bytes and the
		// reply would take 1192 bytes, which do not fit.
		const auto tooLargeHeartbeatRequestChunk = buildHeartbeatRequestChunk(1169);

		test.heartbeatHandler.HandleReceivedHeartbeatRequestChunk(tooLargeHeartbeatRequestChunk.get());

		REQUIRE(test.associationListener.HasSentPackets() == false);
	}

	SECTION("sends heartbeat requests on idle connections")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		test.AdvanceNowMs(test.sctpOptions.heartbeatIntervalMs);

		auto* heartbeatIntervalTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(heartbeatIntervalTimer);
		REQUIRE(heartbeatIntervalTimer->EvaluateHasExpired() == true);
		REQUIRE(test.associationListener.HasSentPackets() == true);

		const std::vector<uint8_t> sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->GetChunksCount() == 1);

		const auto* sentHeartbeatRequestChunk =
		  sentPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();

		REQUIRE(sentHeartbeatRequestChunk);

		const auto* sentHeartbeatInfoParameter =
		  sentHeartbeatRequestChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(sentHeartbeatInfoParameter);
		REQUIRE(sentHeartbeatInfoParameter->HasInfo());

		std::unique_ptr<RTC::SCTP::HeartbeatAckChunk> receivedHeartbeatAckChunk{
			RTC::SCTP::HeartbeatAckChunk::Factory(sctpCommon::FactoryBuffer, test.sctpOptions.mtu)
		};

		auto* receivedHeartbeatInfoParameter =
		  receivedHeartbeatAckChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

		receivedHeartbeatInfoParameter->SetInfo(
		  sentHeartbeatInfoParameter->GetInfo(), sentHeartbeatInfoParameter->GetInfoLength());
		receivedHeartbeatInfoParameter->Consolidate();

		// Respond a while later.
		const int64_t rttMs{ 313 };

		test.tcbContext.ExpectObserveRttUsCalledTimes(1).ExpectObserveRttUsCalledWith(
		  static_cast<int64_t>(rttMs * 1000));

		test.AdvanceNowMs(rttMs);
		test.heartbeatHandler.HandleReceivedHeartbeatAckChunk(receivedHeartbeatAckChunk.get());

		REQUIRE_VERIFICATION_RESULT(test.tcbContext.VerifyExpectations());
	}

	SECTION("doesn't observe RTT on invalid hearbeats receipt")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		test.AdvanceNowMs(test.sctpOptions.heartbeatIntervalMs);

		auto* heartbeatIntervalTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(heartbeatIntervalTimer);
		REQUIRE(heartbeatIntervalTimer->EvaluateHasExpired() == true);
		REQUIRE(test.associationListener.HasSentPackets() == true);

		const std::vector<uint8_t> sentBuffer = test.associationListener.ConsumeFirstSentPacket();

		std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->GetChunksCount() == 1);

		const auto* sentHeartbeatRequestChunk =
		  sentPacket->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();

		REQUIRE(sentHeartbeatRequestChunk);

		const auto* sentHeartbeatInfoParameter =
		  sentHeartbeatRequestChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(sentHeartbeatInfoParameter);
		REQUIRE(sentHeartbeatInfoParameter->HasInfo());

		std::unique_ptr<RTC::SCTP::HeartbeatAckChunk> receivedHeartbeatAckChunk{
			RTC::SCTP::HeartbeatAckChunk::Factory(sctpCommon::FactoryBuffer, test.sctpOptions.mtu)
		};

		auto* receivedHeartbeatInfoParameter =
		  receivedHeartbeatAckChunk->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

		receivedHeartbeatInfoParameter->SetInfo(
		  sentHeartbeatInfoParameter->GetInfo(), sentHeartbeatInfoParameter->GetInfoLength());
		receivedHeartbeatInfoParameter->Consolidate();

		test.tcbContext.ExpectObserveRttUsCalledTimes(0);

		// Go backwards in time to make the HEARTBEAT-ACK have an invalid timestamp
		// in it, as it will be in the future.
		test.AdvanceNowMs(-100);
		test.heartbeatHandler.HandleReceivedHeartbeatAckChunk(receivedHeartbeatAckChunk.get());

		REQUIRE_VERIFICATION_RESULT(test.tcbContext.VerifyExpectations());
	}

	SECTION("increases error if heartbeat request is not acked in time")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		const int64_t rtoMs{ 105 };

		test.tcbContext.WillGetCurrentRtoUsOnce(
		  []()
		  {
			  return static_cast<int64_t>(rtoMs * 1000);
		  });

		test.AdvanceNowMs(test.sctpOptions.heartbeatIntervalMs);

		auto* heartbeatIntervalTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(heartbeatIntervalTimer);
		REQUIRE(heartbeatIntervalTimer->EvaluateHasExpired() == true);

		// Validate that a request was sent.
		REQUIRE(test.associationListener.HasSentPackets() == true);

		test.tcbContext.ExpectIncrementTxErrorCounterCalledTimes(1);

		test.AdvanceNowMs(rtoMs);

		auto* heartbeatTimeoutTimer = test.shared.GetBackoffTimer("sctp-heartbeat-timeout");

		REQUIRE(heartbeatTimeoutTimer);
		REQUIRE(heartbeatTimeoutTimer->EvaluateHasExpired() == true);
		REQUIRE_VERIFICATION_RESULT(test.tcbContext.VerifyExpectations());
	}

	SECTION("doesn't send heartbeat requests when disabled")
	{
		TestHeartbeatHandler test(0);

		test.AdvanceNowMs(test.sctpOptions.heartbeatIntervalMs);

		auto* heartbeatIntervalTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(heartbeatIntervalTimer);
		REQUIRE(heartbeatIntervalTimer->EvaluateHasExpired() == false);
		REQUIRE(test.associationListener.HasSentPackets() == false);
	}
}
