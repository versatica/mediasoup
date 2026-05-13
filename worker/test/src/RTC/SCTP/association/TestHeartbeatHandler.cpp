#include "common.hpp"
#include "mocks/include/MockShared.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include "mocks/include/RTC/SCTP/association/MockTransmissionControlBlockContext.hpp"
#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("SCTP HeartbeatHandler", "[sctp][heartbeathandler]")
{
	constexpr uint64_t InitialNowMs{ 1000000 };
	constexpr uint64_t HeartbeatIntervalMs{ 30000 };

	class TestHeartbeatHandler
	{
	public:
		explicit TestHeartbeatHandler(uint64_t heartbeatIntervalMs)
		  // NOTE: The order in which these members are initialized is **critical**.
		  : sctpOptions(
		      RTC::SCTP::SctpOptions{
		        .heartbeatIntervalMs         = heartbeatIntervalMs,
		        .heartbeatIntervalIncludeRtt = false,
		        .zeroChecksumAlternateErrorDetectionMethod =
		          RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE }),
		    tcbContext(this->associationListener, this->sctpOptions),
		    shared(/*getTimeMs*/
		           [this]()
		           {
			           return this->nowMs;
		           }),
		    heartbeatHandler(
		      this->associationListener,
		      this->sctpOptions,
		      std::addressof(this->shared),
		      std::addressof(this->tcbContext)) {

		    };

	public:
		void AdvanceTime(uint64_t incrementMs)
		{
			this->nowMs += incrementMs;
		}

	private:
		uint64_t nowMs{ InitialNowMs };

		// NOTE: Public members for testing.
	public:
		RTC::SCTP::SctpOptions sctpOptions;
		mocks::RTC::SCTP::MockAssociationListener associationListener;
		mocks::RTC::SCTP::MockTransmissionControlBlockContext tcbContext;
		mocks::MockShared shared;
		RTC::SCTP::HeartbeatHandler heartbeatHandler;
	};

	SECTION("has running heartbeat interval timer")
	{
		TestHeartbeatHandler test(HeartbeatIntervalMs);

		test.tcbContext.SetAssociationEstablished(true);
		test.AdvanceTime(test.sctpOptions.heartbeatIntervalMs);

		auto* backoffTimer = test.shared.GetBackoffTimer("sctp-heartbeat-interval");

		REQUIRE(backoffTimer);
		REQUIRE(backoffTimer->EvaluateHasExpired() == true);
		REQUIRE(test.associationListener.HasSentPackets() == true);

		const std::vector<uint8_t> buffer = test.associationListener.ConsumeFirstSentPacket();

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Parse(buffer.data(), buffer.size()) };

		REQUIRE(packet);
		packet->Dump();
		REQUIRE(packet->GetChunksCount() == 1);

		const auto* heartbeatRequestChunk =
		  packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();

		REQUIRE(heartbeatRequestChunk);

		const auto* heartbeatInfoParameter =
		  heartbeatRequestChunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();

		REQUIRE(heartbeatInfoParameter);
		REQUIRE(heartbeatInfoParameter->HasInfo());
	}

	// TODO: SCTP: More tests.
}
