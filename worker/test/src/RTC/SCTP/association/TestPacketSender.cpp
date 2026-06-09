#include "common.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "mocks/include/RTC/SCTP/association/MockAssociationListener.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace
{
	constexpr uint32_t VerificationTag{ 123 };

	/**
	 * A PacketSender::Listener that records the last `OnPacketSenderPacketSent()`
	 * call.
	 */
	class MockPacketSenderListener : public RTC::SCTP::PacketSender::Listener
	{
	public:
		void OnPacketSenderPacketSent(
		  RTC::SCTP::PacketSender* /*packetSender*/, const RTC::SCTP::Packet* /*packet*/, bool sent) override
		{
			++this->onPacketSentCalls;
			this->lastSent = sent;
		}

	public:
		size_t onPacketSentCalls{ 0 };
		bool lastSent{ false };
	};

	/**
	 * Builds an SCTP packet carrying a single COOKIE-ACK chunk into `buffer`
	 * (which must outlive the returned packet).
	 */
	std::unique_ptr<RTC::SCTP::Packet> buildPacketWithCookieAck(std::vector<uint8_t>& buffer)
	{
		std::unique_ptr<RTC::SCTP::Packet> packet(
		  RTC::SCTP::Packet::Factory(buffer.data(), buffer.size()));

		packet->SetVerificationTag(VerificationTag);

		auto* cookieAckChunk = packet->BuildChunkInPlace<RTC::SCTP::CookieAckChunk>();

		cookieAckChunk->Consolidate();

		return packet;
	}
} // namespace

SCENARIO("SCTP PacketSender", "[sctp][packetsender]")
{
	const RTC::SCTP::SctpOptions sctpOptions;

	MockPacketSenderListener listener;
	mocks::RTC::SCTP::MockAssociationListener associationListener;
	RTC::SCTP::PacketSender packetSender(std::addressof(listener), associationListener);

	SECTION("sends a packet and notifies the listener")
	{
		std::vector<uint8_t> buffer(sctpOptions.mtu);
		const auto packet = buildPacketWithCookieAck(buffer);

		REQUIRE(packetSender.SendPacket(packet.get()) == true);
		REQUIRE(listener.onPacketSentCalls == 1);
		REQUIRE(listener.lastSent == true);
		REQUIRE(associationListener.HasSentPackets() == true);

		// The checksum was written.
		const auto sentBuffer = associationListener.ConsumeFirstSentPacket();
		const std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->ValidateCRC32cChecksum() == true);
	}

	SECTION("reports a failed send to the listener")
	{
		// Simulate that the transport fails to send the packet.
		associationListener.SetSendDataResult(false);

		std::vector<uint8_t> buffer(sctpOptions.mtu);
		const auto packet = buildPacketWithCookieAck(buffer);

		REQUIRE(packetSender.SendPacket(packet.get()) == false);
		REQUIRE(listener.onPacketSentCalls == 1);
		REQUIRE(listener.lastSent == false);
	}

	SECTION("doesn't send a packet without chunks")
	{
		std::vector<uint8_t> buffer(sctpOptions.mtu);
		const std::unique_ptr<RTC::SCTP::Packet> packet(
		  RTC::SCTP::Packet::Factory(buffer.data(), buffer.size()));

		packet->SetVerificationTag(VerificationTag);

		REQUIRE(packetSender.SendPacket(packet.get()) == false);
		REQUIRE(listener.onPacketSentCalls == 0);
		REQUIRE(associationListener.HasSentPackets() == false);
	}

	SECTION("doesn't write the checksum when not requested")
	{
		std::vector<uint8_t> buffer(sctpOptions.mtu);
		const auto packet = buildPacketWithCookieAck(buffer);

		packet->SetChecksum(0);

		REQUIRE(packetSender.SendPacket(packet.get(), /*writeChecksum*/ false) == true);

		const auto sentBuffer = associationListener.ConsumeFirstSentPacket();
		const std::unique_ptr<RTC::SCTP::Packet> sentPacket{ RTC::SCTP::Packet::Parse(
			sentBuffer.data(), sentBuffer.size()) };

		REQUIRE(sentPacket);
		REQUIRE(sentPacket->GetChecksum() == 0);
	}
}
