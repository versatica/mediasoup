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

	SECTION("sequence numbers are given out in order, starting at zero")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == std::nullopt);

		REQUIRE(sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs) == 0);
		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == 0);

		REQUIRE(sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000) == 1);
		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == 1);

		REQUIRE(sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000) == 2);
		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == 2);
	}

	SECTION("a packet is retrieved as it was sent")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t sequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, true, InitialTimeUs);

		const auto retrievedPacket = sendPacketHistory.RetrievePacket(sequenceNumber, true);

		REQUIRE(retrievedPacket.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& sentPacket = retrievedPacket.value();

		REQUIRE(sentPacket.sequenceNumber == sequenceNumber);
		REQUIRE(sentPacket.sendTimeUs == InitialTimeUs);
		REQUIRE(sentPacket.size == PacketSize);
		REQUIRE(sentPacket.audio == true);
	}

	SECTION("a sequence number that was never given out is not resolved")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		REQUIRE(sendPacketHistory.RetrievePacket(0, true) == std::nullopt);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(-1, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(1, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(1000000, true) == std::nullopt);
	}

	SECTION("a packet reported as received is not resolved twice")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t sequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, true).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, true) == std::nullopt);
	}

	SECTION("a packet reported as lost is kept, since it may be reported as received later")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t sequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, false).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, false).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, true).has_value());
		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, true) == std::nullopt);
	}

	SECTION("a packet is resolved by its SSRC and RTP sequence number too")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t sequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);

		const auto sentPacket = sendPacketHistory.RetrievePacket(Ssrc, 100, true);

		REQUIRE(sentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(sentPacket->sequenceNumber == sequenceNumber);

		// The very same RTP sequence number of another stream is another packet.
		REQUIRE(sendPacketHistory.RetrievePacket(Ssrc + 1, 101, true) == std::nullopt);
	}

	SECTION("the latest send of a repeated SSRC and RTP sequence number takes over")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		const int64_t retransmissionSequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs + 10000);

		const auto sentPacket = sendPacketHistory.RetrievePacket(Ssrc, 100, true);

		REQUIRE(sentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(sentPacket->sequenceNumber == retransmissionSequenceNumber);
	}

	SECTION("resolving the first send of a repeated one does not drop the mapping of the latest")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		const int64_t firstSequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		const int64_t retransmissionSequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs + 10000);

		REQUIRE(sendPacketHistory.RetrievePacket(firstSequenceNumber, true).has_value());

		const auto sentPacket = sendPacketHistory.RetrievePacket(Ssrc, 100, true);

		REQUIRE(sentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(sentPacket->sequenceNumber == retransmissionSequenceNumber);
	}

	SECTION("bytes in flight are those of the packets no feedback has reported on")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 3 * PacketSize);

		// Reporting on a packet settles every packet before it as well.
		sendPacketHistory.RetrievePacket(1, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);

		sendPacketHistory.RetrievePacket(2, true);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);
	}

	SECTION("a packet is told the bytes that were in flight when it was sent")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacketHistory.AddPacket(Ssrc, 100, 200, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, 300, false, InitialTimeUs + 10000);

		const auto firstSentPacket = sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(firstSentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(firstSentPacket->dataInFlight == 200);

		const auto secondSentPacket = sendPacketHistory.RetrievePacket(1, false);

		REQUIRE(secondSentPacket.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(secondSentPacket->dataInFlight == 500);
	}

	SECTION("a packet reported as lost stops counting as in flight")
	{
		RTC::BWE::SendPacketHistory sendPacketHistory;

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

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

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + 10000);
		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + 20000);

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

		const int64_t sequenceNumber =
		  sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		// Right at the window it's still held.
		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + WindowDurationUs);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 2 * PacketSize);

		sendPacketHistory.AddPacket(Ssrc, 102, PacketSize, false, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.RetrievePacket(sequenceNumber, true) == std::nullopt);
		REQUIRE(sendPacketHistory.RetrievePacket(Ssrc, 100, true) == std::nullopt);
		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 2 * PacketSize);

		// The sequence numbers of the dropped packets are not given out again.
		REQUIRE(sendPacketHistory.GetLastSequenceNumber() == 2);
	}

	SECTION("the window does not take out the bytes of a packet already reported on")
	{
		constexpr int64_t WindowDurationUs{ 1000000 };

		RTC::BWE::SendPacketHistory sendPacketHistory({ .windowDurationUs = WindowDurationUs });

		sendPacketHistory.AddPacket(Ssrc, 100, PacketSize, false, InitialTimeUs);

		// Reported as lost, so its bytes are gone but it's still held.
		sendPacketHistory.RetrievePacket(0, false);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == 0);

		sendPacketHistory.AddPacket(Ssrc, 101, PacketSize, false, InitialTimeUs + WindowDurationUs + 1);

		REQUIRE(sendPacketHistory.GetOutstandingBytes() == PacketSize);
	}
}
