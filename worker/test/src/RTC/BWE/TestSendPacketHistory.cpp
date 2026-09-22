#include "common.hpp"
#include "RTC/BWE/SendPacketHistory.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE SendPacketHistory", "[bwe][sendpackethistory]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };
	constexpr size_t PacketSize{ 1000 };
	constexpr uint32_t Ssrc{ 1111 };

	// Take note of a packet without it having left yet.
	const auto addPacket = [](
	                         RTC::BWE::SendPacketHistory& sendPacketHistory,
	                         uint16_t seq,
	                         size_t size,
	                         int64_t createdAtUs) -> int64_t
	{
		return sendPacketHistory.AddPacket(
		  { .ssrc = Ssrc, .seq = seq, .size = size, .createdAtUs = createdAtUs });
	};

	// Take note of a packet and of it having left right away, which is what most
	// cases need.
	const auto sendPacket =
	  [&addPacket](
	    RTC::BWE::SendPacketHistory& sendPacketHistory, uint16_t seq, size_t size, int64_t atUs) -> int64_t
	{
		const int64_t sequenceNumber = addPacket(sendPacketHistory, seq, size, atUs);

		sendPacketHistory.ProcessSentPacket(sequenceNumber, atUs);

		return sequenceNumber;
	};

	SECTION("a packet is retrieved as it was sent")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t sequenceNumber = sendPacketHistory.AddPacket(
		  { .ssrc = Ssrc, .seq = 100, .size = PacketSize, .isAudio = true, .createdAtUs = InitialTimeUs });

		REQUIRE(sequenceNumber == 0);

		sendPacketHistory.ProcessSentPacket(sequenceNumber, InitialTimeUs + 500);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(0, true);

		REQUIRE(retrievedEntry.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& entry = retrievedEntry.value();

		REQUIRE(entry.ssrc == Ssrc);
		REQUIRE(entry.seq == 100);
		REQUIRE(entry.createdAtUs == InitialTimeUs);
		REQUIRE(entry.sentPacket.sequenceNumber == 0);
		REQUIRE(entry.sentPacket.size == PacketSize);
		REQUIRE(entry.sentPacket.isAudio == true);
		// The send time is the one it left at, not the one it was taken note of.
		REQUIRE(entry.sentPacket.sendTimeUs == InitialTimeUs + 500);
	}

	SECTION("the burst a packet belongs to survives the history")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const RTC::BWE::Types::ProbeCluster probeCluster{ .id = 7, .minProbes = 5, .minBytes = 4000 };

		const int64_t sequenceNumber = sendPacketHistory.AddPacket(
		  { .ssrc         = Ssrc,
			  .seq          = 100,
			  .size         = PacketSize,
			  .probeCluster = probeCluster,
			  .createdAtUs  = InitialTimeUs });

		sendPacketHistory.ProcessSentPacket(sequenceNumber, InitialTimeUs);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(0, true);

		REQUIRE(retrievedEntry.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& retrievedProbeCluster = retrievedEntry.value().sentPacket.probeCluster;

		REQUIRE(retrievedProbeCluster.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(retrievedProbeCluster.value().id == 7);
	}

	SECTION("a packet that never left is not resolved")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		addPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);

		sendPacketHistory.ProcessSentPacket(0, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true).has_value());
	}

	SECTION("a sequence number that was never taken note of is not resolved")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(-1, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(1, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(1000000, true) == std::nullopt);
	}

	SECTION("a packet reported as received is not resolved twice")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);
	}

	SECTION("a packet reported as lost is kept, since it may be reported as received later")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(0, false).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(0, false).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(0, true).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);
	}

	SECTION("the first report of a loss is told apart from the same loss reported again")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		const auto firstReport = sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(firstReport.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstReport.value().previouslyReportedLost == false);

		const auto secondReport = sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(secondReport.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondReport.value().previouslyReportedLost == true);
	}

	SECTION("a packet is resolved by its SSRC and RTP sequence number too")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);
		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + 10000);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(Ssrc, 100, true);

		REQUIRE(retrievedEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(retrievedEntry.value().sentPacket.sequenceNumber == 0);

		// The very same RTP sequence number of another stream is another packet.
		REQUIRE(sendPacketHistory.RetrievePacket(Ssrc + 1, 101, true) == std::nullopt);
	}

	SECTION("the first send of a repeated SSRC and RTP sequence number keeps the mapping")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);
		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs + 10000);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(Ssrc, 100, true);

		REQUIRE(retrievedEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(retrievedEntry.value().sentPacket.sequenceNumber == 0);
	}

	SECTION("both sends of a repeated SSRC and RTP sequence number have an ambiguous arrival time")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);
		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs + 10000);

		const auto firstEntry = sendPacketHistory.RetrievePacket(0, true);

		REQUIRE(firstEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstEntry.value().ambiguousReceiveTime == true);

		const auto secondEntry = sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(secondEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondEntry.value().ambiguousReceiveTime == true);
	}

	SECTION("a retransmission without RTX is ambiguous although the packet it repeats is gone")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		// Reported as received, so it's not held anymore.
		REQUIRE(sendPacketHistory.RetrievePacket(0, true).has_value());

		// Retransmitted reusing the very same SSRC and RTP sequence number.
		const int64_t sequenceNumber = sendPacketHistory.AddPacket(
		  { .ssrc             = Ssrc,
			  .seq              = 100,
			  .size             = PacketSize,
			  .isRetransmission = true,
			  .originalSsrc     = Ssrc,
			  .createdAtUs      = InitialTimeUs + 10000 });

		sendPacketHistory.ProcessSentPacket(sequenceNumber, InitialTimeUs + 10000);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(retrievedEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(retrievedEntry.value().ambiguousReceiveTime == true);
	}

	SECTION("a retransmission over RTX is not ambiguous, since it goes under its own SSRC")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true).has_value());

		// Retransmitted over RTX, so under another SSRC and sequence number.
		const int64_t sequenceNumber = sendPacketHistory.AddPacket(
		  { .ssrc             = Ssrc + 1,
			  .seq              = 7,
			  .size             = PacketSize,
			  .isRetransmission = true,
			  .originalSsrc     = Ssrc,
			  .createdAtUs      = InitialTimeUs + 10000 });

		sendPacketHistory.ProcessSentPacket(sequenceNumber, InitialTimeUs + 10000);

		const auto retrievedEntry = sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(retrievedEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(retrievedEntry.value().ambiguousReceiveTime == false);
	}

	SECTION("bytes in flight are those of the packets that left and no feedback has reported on")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		// Taking note of a packet is not yet sending it.
		addPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacketHistory.ProcessSentPacket(0, InitialTimeUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);

		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + 10000);
		sendPacket(sendPacketHistory, 102, PacketSize, InitialTimeUs + 20000);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 3 * PacketSize);

		// Reporting on a packet settles every packet before it as well.
		sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);

		sendPacketHistory.RetrievePacket(2, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);
	}

	SECTION("a packet that leaves twice does not count its bytes twice")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		addPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.ProcessSentPacket(0, InitialTimeUs).has_value());
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);

		REQUIRE(sendPacketHistory.ProcessSentPacket(0, InitialTimeUs + 1000) == std::nullopt);
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);
	}

	SECTION("a packet is told the bytes that were in flight when it left")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		addPacket(sendPacketHistory, 100, 200, InitialTimeUs);

		const auto firstSentPacket = sendPacketHistory.ProcessSentPacket(0, InitialTimeUs);

		REQUIRE(firstSentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstSentPacket.value().sequenceNumber == 0);
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstSentPacket.value().dataInFlight == 200);
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 200);

		addPacket(sendPacketHistory, 101, 300, InitialTimeUs + 10000);

		const auto secondSentPacket = sendPacketHistory.ProcessSentPacket(1, InitialTimeUs + 10000);

		REQUIRE(secondSentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondSentPacket.value().dataInFlight == 500);
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 500);

		// And it's what a feedback resolves it into as well.
		const auto firstEntry = sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(firstEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstEntry.value().sentPacket.dataInFlight == 200);

		const auto secondEntry = sendPacketHistory.RetrievePacket(1, false);

		REQUIRE(secondEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondEntry.value().sentPacket.dataInFlight == 500);
	}

	SECTION("bytes that left untracked are attributed to the next packet that leaves")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacketHistory.ProcessSentUntrackedPacket(400, InitialTimeUs);
		sendPacketHistory.ProcessSentUntrackedPacket(600, InitialTimeUs + 1000);

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs + 2000);

		const auto firstEntry = sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(firstEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstEntry.value().sentPacket.priorUnackedData == 1000);

		// They are attributed once, not to every packet that follows.
		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + 3000);

		const auto secondEntry = sendPacketHistory.RetrievePacket(1, false);

		REQUIRE(secondEntry.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondEntry.value().sentPacket.priorUnackedData == 0);
	}

	SECTION("a packet reported as lost stops counting as in flight")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);

		sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		// It's still held, and reporting on it again changes nothing.
		sendPacketHistory.RetrievePacket(0, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);
	}

	SECTION("a feedback arriving out of order does not take bytes out twice")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);
		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + 10000);
		sendPacket(sendPacketHistory, 102, PacketSize, InitialTimeUs + 20000);

		sendPacketHistory.RetrievePacket(2, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacketHistory.RetrievePacket(0, true);
		sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);
	}

	SECTION("packets held for longer than the window are dropped")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		// Right at the window it's still held.
		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + WindowDurationUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 2 * PacketSize);

		sendPacket(sendPacketHistory, 102, PacketSize, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(Ssrc, 100, true) == std::nullopt);
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 2 * PacketSize);

		// Dropping a packet doesn't make the history forget how far the numbers got,
		// so one beyond them is still refused.
		REQUIRE(sendPacketHistory.RetrievePacket(3, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(2, true).has_value());
	}

	SECTION("the window counts from when a packet was taken note of, not from when it left")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });

		addPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		// It took the whole window to leave, which doesn't buy it any more time.
		sendPacketHistory.ProcessSentPacket(0, InitialTimeUs + WindowDurationUs);

		addPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);
	}

	SECTION("the window does not take out the bytes of a packet already reported on")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });

		sendPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		// Reported as lost, so its bytes are gone but it's still held.
		sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);
	}

	SECTION("the window does not take out the bytes of a packet that never left")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });

		addPacket(sendPacketHistory, 100, PacketSize, InitialTimeUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacket(sendPacketHistory, 101, PacketSize, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);
	}
}
