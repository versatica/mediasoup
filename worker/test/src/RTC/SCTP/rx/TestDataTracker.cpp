#include "mocks/include/MockShared.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/rx/DataTracker.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <vector>

SCENARIO("SCTP DataTracker", "[sctp][datatracker]")
{
	class MockBackoffTimerHandleListener : public BackoffTimerHandleInterface::Listener
	{
		/* Pure virtual methods inherited from BackoffTimerHandleInterface::Listener. */
	public:
		void OnBackoffTimer(
		  BackoffTimerHandleInterface* /*backoffTimer*/, uint64_t& /*baseTimeoutMs*/, bool& /*stop*/) override
		{
		}
	};

	constexpr uint32_t Arwnd{ 10000 };
	constexpr uint64_t Mtu{ 1191 };
	constexpr uint32_t InitialTsn{ 11 };

	MockBackoffTimerHandleListener backoffTimerHandleListener;
	uint64_t nowMs{ 10000 };
	mocks::MockShared shared(/*getTimeMs*/
	                         [&nowMs]()
	                         {
		                         return nowMs;
	                         });

	const std::unique_ptr<BackoffTimerHandleInterface> delayedAckTimerUniquePtr{ shared.CreateBackoffTimer(
		BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		  .listener            = std::addressof(backoffTimerHandleListener),
		  .label               = "mock-sctp-delayed-ack",
		  .baseTimeoutMs       = 0,
		  .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		  .maxBackoffTimeoutMs = std::nullopt,
		  .maxRestarts         = 0 }) };

	auto* delayedAckTimer = delayedAckTimerUniquePtr.get();

	RTC::SCTP::DataTracker dataTracker(delayedAckTimer, InitialTsn);

	auto createPacket = []()
	{
		return std::unique_ptr<RTC::SCTP::Packet>{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, Mtu) };
	};

	auto addSackChunk = [&dataTracker](RTC::SCTP::Packet* packet, size_t aRwnd)
	{
		dataTracker.AddSackSelectiveAck(packet, aRwnd);

		const auto* sackChunk = packet->GetFirstChunkOfType<RTC::SCTP::SackChunk>();

		return sackChunk;
	};

	auto observe = [&dataTracker](std::initializer_list<uint32_t> tsns, bool expectAsDuplicate = false)
	{
		for (const uint32_t tsn : tsns)
		{
			if (expectAsDuplicate)
			{
				REQUIRE(dataTracker.Observe(tsn, /*immediateAck*/ false) == false);
			}
			else
			{
				REQUIRE(dataTracker.Observe(tsn, /*immediateAck*/ false) == true);
			}
		}
	};

	SECTION("empty")
	{
		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("observe single in order packet")
	{
		observe({ 11 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 11);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("observe many in order moves cumulative TSN ack")
	{
		observe({ 11, 12, 13 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 13);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("observe out of order moves cumulative TSN ack")
	{
		observe({ 12, 13, 14, 11 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("single gap")
	{
		observe({ 12 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 }
    });
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("example from RFC 9260 section 3.3.4")
	{
		observe({ 11, 12, 14, 15, 17 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 12);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 3 },
                                        { 5, 5 }
    });
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("ack already received chunks")
	{
		observe({ 11 });

		const auto packet1     = createPacket();
		const auto* sackChunk1 = addSackChunk(packet1.get(), Arwnd);

		REQUIRE(sackChunk1);
		REQUIRE(sackChunk1->GetCumulativeTsnAck() == 11);
		REQUIRE(sackChunk1->GetGapAckBlocks().empty());
		REQUIRE(sackChunk1->GetDuplicateTsns().empty());

		// Receive old chunk.
		observe({ 8 }, /*expectAsDuplicate*/ true);

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), Arwnd);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetCumulativeTsnAck() == 11);
		REQUIRE(sackChunk2->GetGapAckBlocks().empty());
		REQUIRE(sackChunk2->GetDuplicateTsns().empty());
	}

	SECTION("double send retransmitted chunk")
	{
		observe({ 11, 13, 14, 15 });

		const auto packet1     = createPacket();
		const auto* sackChunk1 = addSackChunk(packet1.get(), Arwnd);

		REQUIRE(sackChunk1);
		REQUIRE(sackChunk1->GetCumulativeTsnAck() == 11);
		REQUIRE(
		  sackChunk1->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                     { 2, 4 },
    });
		REQUIRE(sackChunk1->GetDuplicateTsns().empty());

		// Fill in the hole.
		observe({ 12, 16, 17, 18 });

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), Arwnd);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetCumulativeTsnAck() == 18);
		REQUIRE(sackChunk2->GetGapAckBlocks().empty());
		REQUIRE(sackChunk2->GetDuplicateTsns().empty());

		// Receive chunk 12 again.
		observe({ 12 }, /*expectAsDuplicate*/ true);
		observe({ 19, 20, 21 });

		const auto packet3     = createPacket();
		const auto* sackChunk3 = addSackChunk(packet3.get(), Arwnd);

		REQUIRE(sackChunk3);
		REQUIRE(sackChunk3->GetCumulativeTsnAck() == 21);
		REQUIRE(sackChunk3->GetGapAckBlocks().empty());
		REQUIRE(sackChunk3->GetDuplicateTsns().empty());
	}

	SECTION("forward tsn simple")
	{
		// Messages (11, 12, 13), (14, 15) - first message expires.
		observe({ 11, 12, 15 });

		dataTracker.HandleForwardTsn(13);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 13);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 },
    });
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}
}
