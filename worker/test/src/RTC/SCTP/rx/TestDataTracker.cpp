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

	SECTION("observe many in order moves cumulative tsn ack")
	{
		observe({ 11, 12, 13 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 13);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("observe out of order moves cumulative tsn ack")
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

		// Receive old chunk.
		observe({ 8 }, /*expectAsDuplicate*/ true);

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), Arwnd);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetCumulativeTsnAck() == 11);
		REQUIRE(sackChunk2->GetGapAckBlocks().empty());
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

		// Fill in the hole.
		observe({ 12, 16, 17, 18 });

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), Arwnd);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetCumulativeTsnAck() == 18);
		REQUIRE(sackChunk2->GetGapAckBlocks().empty());

		// Receive chunk 12 again.
		observe({ 12 }, /*expectAsDuplicate*/ true);
		observe({ 19, 20, 21 });

		const auto packet3     = createPacket();
		const auto* sackChunk3 = addSackChunk(packet3.get(), Arwnd);

		REQUIRE(sackChunk3);
		REQUIRE(sackChunk3->GetCumulativeTsnAck() == 21);
		REQUIRE(sackChunk3->GetGapAckBlocks().empty());
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

	SECTION("forward tsn skips from gap block")
	{
		// Messages (11, 12, 13), (14, 15) - first message expires.
		observe({ 11, 12, 14 });

		dataTracker.HandleForwardTsn(13);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("example from RFC 3758")
	{
		dataTracker.HandleForwardTsn(102);

		observe({ 102 }, /*expectAsDuplicate*/ true);
		observe({ 104, 105, 107 });

		dataTracker.HandleForwardTsn(103);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 105);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 },
    });
	}

	SECTION("empty all acks")
	{
		observe({ 11, 13, 14, 15 });

		dataTracker.HandleForwardTsn(100);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 100);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().empty());
	}

	SECTION("sets arwnd correctly")
	{
		const auto packet1     = createPacket();
		const auto* sackChunk1 = addSackChunk(packet1.get(), 100);

		REQUIRE(sackChunk1);
		REQUIRE(sackChunk1->GetAdvertisedReceiverWindowCredit() == 100);

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), 101);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetAdvertisedReceiverWindowCredit() == 101);
	}

	SECTION("will increase cumulative ack tsn")
	{
		REQUIRE(dataTracker.GetLastCumulativeAckedTsn() == 10);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(10) == false);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(11) == true);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(12) == false);

		observe({ 11, 12, 13, 14, 15 });

		REQUIRE(dataTracker.GetLastCumulativeAckedTsn() == 15);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(15) == false);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(16) == true);
		REQUIRE(dataTracker.WillIncreaseCumAckTsn(17) == false);
	}

	SECTION("force should send sack immediately")
	{
		REQUIRE(dataTracker.ShouldSendAck() == false);

		dataTracker.ForceImmediateSack();

		REQUIRE(dataTracker.ShouldSendAck() == true);
	}

	SECTION("will accept valid tsns")
	{
		// The initial TSN is always one more than the last, which is our base.
		const uint32_t lastTsn = InitialTsn - 1;
		const int limit = static_cast<int>(RTC::SCTP::DataTracker::MaxAcceptedOutstandingFragments);

		for (int i{ -limit }; i <= limit; ++i)
		{
			REQUIRE(dataTracker.IsTsnValid(lastTsn + i) == true);
		}
	}

	SECTION("will not accept invalid tsns")
	{
		// The initial TSN is always one more than the last, which is our base.
		const uint32_t lastTsn = InitialTsn - 1;
		const uint32_t limit   = RTC::SCTP::DataTracker::MaxAcceptedOutstandingFragments;

		REQUIRE(dataTracker.IsTsnValid(lastTsn + limit + 1) == false);
		REQUIRE(dataTracker.IsTsnValid(lastTsn - (limit + 1)) == false);
		REQUIRE(dataTracker.IsTsnValid(lastTsn + 0x8000000) == false);
		REQUIRE(dataTracker.IsTsnValid(lastTsn - 0x8000000) == false);
	}

	SECTION("report single duplicate tsns")
	{
		observe({ 11, 12 });
		observe({ 11 }, /*expectAsDuplicate*/ true);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 12);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns() == std::vector<uint32_t>{ 11 });
	}

	SECTION("report multiple duplicate tsns")
	{
		observe({ 11, 12, 13, 14 });
		observe({ 12, 13, 12, 13 }, /*expectAsDuplicate*/ true);
		observe({ 15, 16 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 16);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns() == std::vector<uint32_t>{ 12, 13 });
	}

	SECTION("report duplicate tsns in gap-ack-blocks")
	{
		observe({ 11, /*12,*/ 13, 14 });
		observe({ 13, 14 }, /*expectAsDuplicate*/ true);
		observe({ 15, 16 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 11);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 5 },
    });
		REQUIRE(sackChunk->GetDuplicateTsns() == std::vector<uint32_t>{ 13, 14 });
	}

	SECTION("clears duplicate tsns after creating sack")
	{
		observe({ 11, 12, 13, 14 });
		observe({ 12, 13, 12, 13 }, /*expectAsDuplicate*/ true);
		observe({ 15, 16 });

		const auto packet1     = createPacket();
		const auto* sackChunk1 = addSackChunk(packet1.get(), Arwnd);

		REQUIRE(sackChunk1);
		REQUIRE(sackChunk1->GetCumulativeTsnAck() == 16);
		REQUIRE(sackChunk1->GetGapAckBlocks().empty());
		REQUIRE(sackChunk1->GetDuplicateTsns() == std::vector<uint32_t>{ 12, 13 });

		observe({ 17 });

		const auto packet2     = createPacket();
		const auto* sackChunk2 = addSackChunk(packet2.get(), Arwnd);

		REQUIRE(sackChunk2);
		REQUIRE(sackChunk2->GetCumulativeTsnAck() == 17);
		REQUIRE(sackChunk2->GetGapAckBlocks().empty());
		REQUIRE(sackChunk2->GetDuplicateTsns().empty());
	}

	SECTION("limits number of duplicates reported")
	{
		for (size_t i{ 0 }; i < RTC::SCTP::DataTracker::MaxDuplicateTsnReported + 10; ++i)
		{
			const uint32_t tsn = 11 + static_cast<uint32_t>(i);

			dataTracker.Observe(tsn, /*immediateAck*/ false);
			dataTracker.Observe(tsn, /*immediateAck*/ false);
		}

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
		REQUIRE(sackChunk->GetDuplicateTsns().size() == RTC::SCTP::DataTracker::MaxDuplicateTsnReported);
	}

	SECTION("limits number of gap-ack-blocks reported")
	{
		for (size_t i{ 0 }; i < RTC::SCTP::DataTracker::MaxGapAckBlocksReported + 10; ++i)
		{
			const uint32_t tsn = 11 + static_cast<uint32_t>(i * 2);

			dataTracker.Observe(tsn, /*immediateAck*/ false);
		}

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 11);
		REQUIRE(sackChunk->GetGapAckBlocks().size() == RTC::SCTP::DataTracker::MaxGapAckBlocksReported);
	}

	SECTION("sends sack for first packet observed")
	{
		observe({ 11 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);

		const auto* backoffTimer = shared.GetBackoffTimer("mock-sctp-delayed-ack");

		REQUIRE(backoffTimer);
		REQUIRE(backoffTimer->IsRunning() == false);
	}

	SECTION("sends sack every second packet when there is no packet loss")
	{
		auto* backoffTimer = shared.GetBackoffTimer("mock-sctp-delayed-ack");

		REQUIRE(backoffTimer);

		observe({ 11 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 12 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == false);
		REQUIRE(backoffTimer->IsRunning() == true);

		observe({ 13 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 14 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == false);
		REQUIRE(backoffTimer->IsRunning() == true);

		observe({ 15 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);
	}

	SECTION("sends sack every packet on packet loss")
	{
		const auto* backoffTimer = shared.GetBackoffTimer("mock-sctp-delayed-ack");

		REQUIRE(backoffTimer);

		observe({ 11 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 13 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 14 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 15 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 16 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		// Fill the hole.
		observe({ 12 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == false);
		REQUIRE(backoffTimer->IsRunning() == true);

		// Goes back to every second packet.
		observe({ 17 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 18 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == false);
		REQUIRE(backoffTimer->IsRunning() == true);
	}

	SECTION("sends sack on duplicate data chunks")
	{
		const auto* backoffTimer = shared.GetBackoffTimer("mock-sctp-delayed-ack");

		REQUIRE(backoffTimer);

		observe({ 11 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 11 }, /*expectAsDuplicate*/ true);
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		observe({ 12 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == false);
		REQUIRE(backoffTimer->IsRunning() == true);

		// Goes back to every second packet.
		observe({ 13 });
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);

		// Duplicate again.
		observe({ 12 }, /*expectAsDuplicate*/ true);
		dataTracker.ObservePacketEnd();

		REQUIRE(dataTracker.ShouldSendAck() == true);
		REQUIRE(backoffTimer->IsRunning() == false);
	}

	SECTION("gap-ack-block add single block")
	{
		observe({ 12 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 },
    });
	}

	SECTION("gap-ack-block adds another")
	{
		observe({ 12 });
		observe({ 14 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 },
		                                    { 4, 4 },
    });
	}

	SECTION("gap-ack-block adds duplicate")
	{
		observe({ 12 });
		observe({ 12 }, /*expectAsDuplicate*/ true);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 2 },
    });
		REQUIRE(sackChunk->GetDuplicateTsns() == std::vector<uint32_t>{ 12 });
	}

	SECTION("gap-ack-block expands to right")
	{
		observe({ 12 });
		observe({ 13 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 3 },
    });
	}

	SECTION("gap-ack-block expands to right with other")
	{
		observe({ 12 });
		observe({ 20 });
		observe({ 30 });
		observe({ 21 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2,  2  },
		                                    { 10, 11 },
		                                    { 20, 20 },
    });
	}

	SECTION("gap-ack-block expands to left")
	{
		observe({ 13 });
		observe({ 12 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2, 3 },
    });
	}

	SECTION("gap-ack-block expands to left with other")
	{
		observe({ 12 });
		observe({ 21 });
		observe({ 30 });
		observe({ 20 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2,  2  },
		                                    { 10, 11 },
		                                    { 20, 20 },
    });
	}

	SECTION("gap-ack-block expands to right and merges")
	{
		observe({ 12 });
		observe({ 20 });
		observe({ 22 });
		observe({ 30 });
		observe({ 21 });

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2,  2  },
		                                    { 10, 12 },
		                                    { 20, 20 },
    });
	}

	SECTION("gap-ack-block merges many blocks into one")
	{
		auto getGapAckBlocks = [&]()
		{
			const auto packet     = createPacket();
			const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

			REQUIRE(sackChunk);

			return sackChunk->GetGapAckBlocks();
		};

		observe({ 22 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 }
    });

		observe({ 30 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 },
		                         { 20, 20 },
    });

		observe({ 24 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 },
		                         { 14, 14 },
		                         { 20, 20 },
    });

		observe({ 28 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 },
		                         { 14, 14 },
		                         { 18, 18 },
		                         { 20, 20 },
    });

		observe({ 26 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 },
		                         { 14, 14 },
		                         { 16, 16 },
		                         { 18, 18 },
		                         { 20, 20 },
    });

		observe({ 29 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 12 },
		                         { 14, 14 },
		                         { 16, 16 },
		                         { 18, 20 },
    });

		observe({ 23 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 14 },
		                         { 16, 16 },
		                         { 18, 20 },
    });

		observe({ 27 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 14 },
		                         { 16, 20 },
    });

		observe({ 25 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 12, 20 }
    });

		observe({ 20 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 10, 10 },
		                         { 12, 20 },
    });

		observe({ 32 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 10, 10 },
		                         { 12, 20 },
		                         { 22, 22 },
    });

		observe({ 21 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 10, 20 },
		                         { 22, 22 },
    });

		observe({ 31 });

		REQUIRE(
		  getGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                         { 10, 22 }
    });
	}

	SECTION("gap-ack-block remove before cumulative ack tsn")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(8);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 10);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2,  4  },
		                                    { 10, 12 },
		                                    { 20, 21 },
    });
	}

	SECTION("gap-ack-block remove before first block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(11);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 6,  8  },
		                                    { 16, 17 },
    });
	}

	SECTION("gap-ack-block remove at beginning of first block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(12);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 6,  8  },
		                                    { 16, 17 },
    });
	}

	SECTION("gap-ack-block remove at middle of first block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(13);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 6,  8  },
		                                    { 16, 17 },
    });
	}

	SECTION("gap-ack-block remove at end of first block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(14);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 14);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 6,  8  },
		                                    { 16, 17 },
    });
	}

	SECTION("gap-ack-block remove right after first block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(18);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 18);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 2,  4  },
		                                    { 12, 13 },
    });
	}

	SECTION("gap-ack-block remove right before second block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(19);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 22);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 8, 9 },
    });
	}

	SECTION("gap-ack-block remove right at start of second block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(20);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 22);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 8, 9 },
    });
	}

	SECTION("gap-ack-block remove right at middle of second block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(21);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 22);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 8, 9 },
    });
	}

	SECTION("gap-ack-block remove right at end of second block")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(22);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 22);
		REQUIRE(
		  sackChunk->GetGapAckBlocks() == std::vector<RTC::SCTP::SackChunk::GapAckBlock>{
		                                    { 8, 9 },
    });
	}

	SECTION("gap-ack-block remove far after all blocks")
	{
		observe({ 12, 13, 14, 20, 21, 22, 30, 31 });

		dataTracker.HandleForwardTsn(40);

		const auto packet     = createPacket();
		const auto* sackChunk = addSackChunk(packet.get(), Arwnd);

		REQUIRE(sackChunk);
		REQUIRE(sackChunk->GetCumulativeTsnAck() == 40);
		REQUIRE(sackChunk->GetGapAckBlocks().empty());
	}

	SECTION("does not accept data before forward tsn")
	{
		observe({ 12, 13, 14, 15, 17 });
		dataTracker.ObservePacketEnd();

		dataTracker.HandleForwardTsn(13);

		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == false);
	}

	SECTION("does not accept data at forward tsn")
	{
		observe({ 12, 13, 14, 15, 17 });

		dataTracker.ObservePacketEnd();
		dataTracker.HandleForwardTsn(16);

		REQUIRE(dataTracker.Observe(16, /*immediateAck*/ false) == false);
	}

	SECTION("does not accept data before cumulative ack tsn")
	{
		REQUIRE(InitialTsn == 11);

		REQUIRE(dataTracker.Observe(10, /*immediateAck*/ false) == false);
	}

	SECTION("does not accept contiguous duplicate data")
	{
		REQUIRE(InitialTsn == 11);

		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == false);
		REQUIRE(dataTracker.Observe(12, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(12, /*immediateAck*/ false) == false);
		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == false);
		REQUIRE(dataTracker.Observe(10, /*immediateAck*/ false) == false);
	}

	SECTION("does not accept gaps with duplicate data")
	{
		REQUIRE(InitialTsn == 11);

		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(11, /*immediateAck*/ false) == false);

		REQUIRE(dataTracker.Observe(14, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(14, /*immediateAck*/ false) == false);

		REQUIRE(dataTracker.Observe(13, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(13, /*immediateAck*/ false) == false);

		REQUIRE(dataTracker.Observe(12, /*immediateAck*/ false) == true);
		REQUIRE(dataTracker.Observe(12, /*immediateAck*/ false) == false);
	}
}
