#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/TransportWideCcPacket.hpp"
#include "Utils.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::abs()
#include <cstring> // std::memset()
#include <limits>  // std::numeric_limits
#include <vector>

SCENARIO("RTCP Transport-wide CC Feedback Packet (205, FMT 15)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	// Length (in bytes) of the fixed header of the packet.
	static constexpr size_t HeaderSize{ 20 };
	// Length (in bytes) of a packet chunk.
	static constexpr size_t StatusChunkSize{ 2 };
	// Length (in bytes) of a small receive delta.
	static constexpr size_t SmallDeltaSize{ 1 };
	// Length (in bytes) of a large receive delta.
	static constexpr size_t LargeDeltaSize{ 2 };
	// Largest receive delta (in microseconds) that fits in a small receive
	// delta.
	static constexpr int64_t DeltaLimitUs{ 0xFF * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs };
	// The reference time is expressed in multiples of this (in microseconds).
	static constexpr int64_t BaseTimeTickUs{ RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs *
	                                         (1 << 8) };
	// Period (in microseconds) after which the reference time wraps around.
	static constexpr int64_t BaseTimeWrapPeriodUs{ BaseTimeTickUs * (1 << 24) };

	/**
	 * Status of an RTP packet as given by ForAllRtpPackets().
	 */
	struct RtpPacketStatus
	{
		/**
		 * Transport-wide sequence number.
		 */
		uint16_t sequenceNumber;
		/**
		 * Receive time relative to the reference time, or `std::nullopt` if not
		 * received.
		 */
		std::optional<int64_t> deltaSinceBaseUs;
	};

	const auto getRtpPacketStatuses =
	  [](const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> std::vector<RtpPacketStatus>
	{
		std::vector<RtpPacketStatus> rtpPacketStatuses;

		packet->ForAllRtpPackets(
		  [&rtpPacketStatuses](uint16_t sequenceNumber, std::optional<int64_t> deltaSinceBaseUs) -> void
		  {
			  rtpPacketStatuses.push_back(
			    { .sequenceNumber = sequenceNumber, .deltaSinceBaseUs = deltaSinceBaseUs });
		  });

		return rtpPacketStatuses;
	};

	const auto getNumberOfLostRtpPackets =
	  [&getRtpPacketStatuses](const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> size_t
	{
		size_t numberOfLostRtpPackets{ 0 };

		for (const auto& rtpPacketStatus : getRtpPacketStatuses(packet))
		{
			if (!rtpPacketStatus.deltaSinceBaseUs.has_value())
			{
				++numberOfLostRtpPackets;
			}
		}

		return numberOfLostRtpPackets;
	};

	SECTION("TransportWideCcPacket::Parse() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> void
		{
			const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

			REQUIRE(
			  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::TCC);
			REQUIRE(packet->HasUnknownMessageType() == false);
			REQUIRE(packet->GetSenderSsrc() == 0xFA17FA17);
			REQUIRE(packet->GetMediaSsrc() == 0x09FAFF67);
			REQUIRE(packet->GetBaseSequenceNumber() == 39);
			REQUIRE(packet->GetPacketStatusCount() == 13);
			REQUIRE(
			  packet->GetBaseTimeUs() ==
			  BaseTimeWrapPeriodUs + (static_cast<int64_t>(6275825) * BaseTimeTickUs));
			REQUIRE(packet->GetFeedbackPacketCount() == 3);
			REQUIRE(packet->HasDeltas() == true);
			REQUIRE(receivedRtpPackets.size() == 10);

			REQUIRE(receivedRtpPackets[0].sequenceNumber == 39);
			REQUIRE(receivedRtpPackets[0].deltaTicks == 16);
			REQUIRE(receivedRtpPackets[1].sequenceNumber == 40);
			REQUIRE(receivedRtpPackets[1].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[2].sequenceNumber == 41);
			REQUIRE(receivedRtpPackets[2].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[3].sequenceNumber == 42);
			REQUIRE(receivedRtpPackets[3].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[4].sequenceNumber == 43);
			REQUIRE(receivedRtpPackets[4].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[5].sequenceNumber == 44);
			REQUIRE(receivedRtpPackets[5].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[6].sequenceNumber == 45);
			REQUIRE(receivedRtpPackets[6].deltaTicks == 0);
			REQUIRE(receivedRtpPackets[7].sequenceNumber == 49);
			REQUIRE(receivedRtpPackets[7].deltaTicks == 28);
			REQUIRE(receivedRtpPackets[8].sequenceNumber == 50);
			REQUIRE(receivedRtpPackets[8].deltaTicks == 4);
			REQUIRE(receivedRtpPackets[9].sequenceNumber == 51);
			REQUIRE(receivedRtpPackets[9].deltaTicks == 0);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 7
			0x8F, 0xCD, 0x00, 0x07,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 0x09FAFF67
			0x09, 0xFA, 0xFF, 0x67,
			// Base sequence number: 39, Packet status count: 13
			0x00, 0x27, 0x00, 0x0D,
			// Reference time: 6275825, Feedback packet count: 3
			0x5F, 0xC2, 0xF1, 0x03,
			// One Bit Status Vector Chunk: 1111111000111(0)
			0xBF, 0x8E,
			// Receive deltas: 16, 0, 0, 0, 0, 0, 0, 28, 4, 0
			0x10, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x1C, 0x04, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());

		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			0x8F, 0xCD, 0x00, 0x07,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x0D,
			0x5F, 0xC2, 0xF1, 0x03,
			0xBF, 0x8E, 0x10, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x1C, 0x04, 0x00
		};
		// clang-format on

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkPacket(packet.get());
	}

	SECTION("TransportWideCcPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 5
			0xAF, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 1234
			0x00, 0x00, 0x04, 0xD2,
			// Base sequence number: 13, Packet status count: 1
			0x00, 0x0D, 0x00, 0x01,
			// Reference time: 35504, Feedback packet count: 0
			0x00, 0x8A, 0xB0, 0x00,
			// Run Length Chunk: symbol 1, run length 1
			0x20, 0x01,
			// Receive delta: 228
			0xE4,
			// Padding: 1 byte
			0x01
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 1);

		const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

		REQUIRE(packet->GetSenderSsrc() == 0xFA17FA17);
		REQUIRE(packet->GetMediaSsrc() == 1234);
		REQUIRE(packet->GetBaseSequenceNumber() == 13);
		REQUIRE(packet->GetPacketStatusCount() == 1);
		REQUIRE(packet->GetBaseTimeUs() == 1076014080000);
		REQUIRE(packet->GetFeedbackPacketCount() == 0);
		REQUIRE(packet->HasDeltas() == true);
		REQUIRE(receivedRtpPackets.size() == 1);
		REQUIRE(receivedRtpPackets[0].sequenceNumber == 13);
		REQUIRE(receivedRtpPackets[0].deltaTicks == 228);
	}

	SECTION("TransportWideCcPacket::Parse() with zero padding and P bit not set succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 5
			0x8F, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Base sequence number: 0, Packet status count: 1
			0x00, 0x00, 0x00, 0x01,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// Run Length Chunk: symbol 1, run length 1
			0x20, 0x01,
			// Receive delta: 0
			0x00,
			// Zero padding (P bit not set)
			0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

		REQUIRE(packet->HasDeltas() == true);
		REQUIRE(receivedRtpPackets.size() == 1);
		REQUIRE(receivedRtpPackets[0].sequenceNumber == 0);
		REQUIRE(receivedRtpPackets[0].deltaTicks == 0);
	}

	SECTION("TransportWideCcPacket::Parse() with more padding than needed succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 6
			0xAF, 0xCD, 0x00, 0x06,
			// SSRC of packet sender: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Base sequence number: 0, Packet status count: 1
			0x00, 0x00, 0x00, 0x01,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// Run Length Chunk: symbol 1, run length 1
			0x20, 0x01,
			// Receive delta: 0
			0x00,
			// Padding: 5 bytes
			0x00,
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 5);

		const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

		REQUIRE(packet->HasDeltas() == true);
		REQUIRE(receivedRtpPackets.size() == 1);
		REQUIRE(receivedRtpPackets[0].sequenceNumber == 0);
		REQUIRE(receivedRtpPackets[0].deltaTicks == 0);
	}

	SECTION("TransportWideCcPacket::Parse() without receive deltas succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 5
			0xAF, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Base sequence number: 1, Packet status count: 13
			0x00, 0x01, 0x00, 0x0D,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// One Bit Status Vector Chunk: 1100001111001(0)
			0xB0, 0xF2,
			// Padding: 2 bytes
			0x00, 0x02
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

		REQUIRE(packet->GetBaseSequenceNumber() == 1);
		REQUIRE(packet->GetPacketStatusCount() == 13);
		REQUIRE(packet->HasDeltas() == false);
		REQUIRE(receivedRtpPackets.size() == 7);
		REQUIRE(receivedRtpPackets[0].sequenceNumber == 1);
		REQUIRE(receivedRtpPackets[1].sequenceNumber == 2);
		REQUIRE(receivedRtpPackets[2].sequenceNumber == 7);
		REQUIRE(receivedRtpPackets[3].sequenceNumber == 8);
		REQUIRE(receivedRtpPackets[4].sequenceNumber == 9);
		REQUIRE(receivedRtpPackets[5].sequenceNumber == 10);
		REQUIRE(receivedRtpPackets[6].sequenceNumber == 13);

		for (const auto& receivedRtpPacket : receivedRtpPackets)
		{
			REQUIRE(receivedRtpPacket.deltaTicks == 0);
		}
	}

	SECTION("TransportWideCcPacket::Parse() with no packet status succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 4
			0x8F, 0xCD, 0x00, 0x04,
			// SSRC of packet sender: 0xFA17FA17
			0xFA, 0x17, 0xFA, 0x17,
			// SSRC of media source: 0x09FAFF67
			0x09, 0xFA, 0xFF, 0x67,
			// Base sequence number: 39, Packet status count: 0
			0x00, 0x27, 0x00, 0x00,
			// Reference time: -2, Feedback packet count: 1
			0xFF, 0xFF, 0xFE, 0x01
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetBaseSequenceNumber() == 39);
		REQUIRE(packet->GetPacketStatusCount() == 0);
		// The Reference time field is a 24 bits signed integer.
		REQUIRE(
		  packet->GetBaseTimeUs() == BaseTimeWrapPeriodUs + (static_cast<int64_t>(-2) * BaseTimeTickUs));
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(packet->GetReceivedRtpPackets().empty());
		REQUIRE(getRtpPacketStatuses(packet.get()).empty());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			0x8F, 0xCD, 0x00, 0x04,
			0xFA, 0x17, 0xFA, 0x17,
			0x09, 0xFA, 0xFF, 0x67,
			0x00, 0x27, 0x00, 0x00,
			0xFF, 0xFF, 0xFE, 0x01
		};
		// clang-format on

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("TransportWideCcPacket::Parse() fails")
	{
		// No space for the fixed header.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 3
			0x8F, 0xCD, 0x00, 0x03,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 0
			0x00, 0x01, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer1, sizeof(buffer1)));

		// Packet chunks do not cover the packet status count.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 5
			0x8F, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 20
			0x00, 0x01, 0x00, 0x14,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// One Bit Status Vector Chunk: 14 x received
			0xBF, 0xFF,
			// Run Length Chunk: symbol 0, run length 0
			0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer2, sizeof(buffer2)));

		// Invalid delta size (reserved symbol 3) with receive deltas included.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, FMT=15, PT:205, Length: 6
			0x8F, 0xCD, 0x00, 0x06,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 1
			0x00, 0x01, 0x00, 0x01,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// Two Bit Status Vector Chunk: symbol 3
			0xF0, 0x00,
			// 3 bytes for the receive delta
			0x01, 0x02,
			0x03, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer3, sizeof(buffer3)));

		// Wrong message type (FMT 1, NACK).
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=0, FMT=1, PT:205, Length: 4
			0x81, 0xCD, 0x00, 0x04,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 0
			0x00, 0x01, 0x00, 0x00,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer4, sizeof(buffer4)));

		// Wrong packet type (PT 206, PSFB).
		// clang-format off
		alignas(4) uint8_t buffer5[] =
		{
			// V=2, P=0, FMT=15, PT:206, Length: 4
			0x8F, 0xCE, 0x00, 0x04,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1, Packet status count: 0
			0x00, 0x01, 0x00, 0x00,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer5, sizeof(buffer5)));
	}

	SECTION("TransportWideCcPacket::Factory() succeeds")
	{
		const auto checkPacket = [](const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> void
		{
			const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

			REQUIRE(
			  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::TCC);
			REQUIRE(packet->GetSenderSsrc() == 1111);
			REQUIRE(packet->GetMediaSsrc() == 2222);
			REQUIRE(packet->GetBaseSequenceNumber() == 1000);
			REQUIRE(packet->GetPacketStatusCount() == 4);
			REQUIRE(packet->GetBaseTimeUs() == 1000000000000);
			REQUIRE(packet->GetFeedbackPacketCount() == 3);
			REQUIRE(packet->HasDeltas() == true);
			REQUIRE(receivedRtpPackets.size() == 3);

			REQUIRE(receivedRtpPackets[0].sequenceNumber == 1000);
			REQUIRE(receivedRtpPackets[0].deltaTicks == 4);
			REQUIRE(receivedRtpPackets[1].sequenceNumber == 1001);
			REQUIRE(receivedRtpPackets[1].deltaTicks == 4);
			REQUIRE(receivedRtpPackets[2].sequenceNumber == 1003);
			REQUIRE(receivedRtpPackets[2].deltaTicks == 4);
		};

		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 6
			0xAF, 0xCD, 0x00, 0x06,
			// SSRC of packet sender: 1111
			0x00, 0x00, 0x04, 0x57,
			// SSRC of media source: 2222
			0x00, 0x00, 0x08, 0xAE,
			// Base sequence number: 1000, Packet status count: 4
			0x03, 0xE8, 0x00, 0x04,
			// Reference time: -1152216, Feedback packet count: 3
			0xEE, 0x6B, 0x28, 0x03,
			// Two Bit Status Vector Chunk: 1, 1, 0, 1
			0xD4, 0x40,
			// Receive deltas: 4, 4, 4
			0x04, 0x04,
			0x04,
			// Padding: 3 bytes
			0x00, 0x00, 0x03
		};
		// clang-format on

		// Dirty the buffer to check that Factory() writes zeros in the header.
		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(
		  packet->GetMessageType() == RTC::NEW_RTCP::AnyTransportLayerFeedbackPacket::MessageType::TCC);
		REQUIRE(packet->GetSenderSsrc() == 0);
		REQUIRE(packet->GetMediaSsrc() == 0);
		REQUIRE(packet->GetBaseSequenceNumber() == 0);
		REQUIRE(packet->GetPacketStatusCount() == 0);
		REQUIRE(packet->GetBaseTimeUs() == BaseTimeWrapPeriodUs);
		REQUIRE(packet->GetFeedbackPacketCount() == 0);
		REQUIRE(packet->HasDeltas() == true);
		REQUIRE(packet->GetReceivedRtpPackets().empty());

		/* Modify it. */

		packet->SetSenderSsrc(1111);
		packet->SetMediaSsrc(2222);
		packet->SetFeedbackPacketCount(3);
		packet->SetBase(1000, 1000000000000);

		REQUIRE(packet->AddReceivedRtpPacket(1000, 1000000001000));
		REQUIRE(packet->AddReceivedRtpPacket(1001, 1000000002000));
		REQUIRE(packet->AddReceivedRtpPacket(1003, 1000000003000));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		checkPacket(packet.get());

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);

		/* Parse it. */

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(expectedBuffer, sizeof(expectedBuffer))
		};

		REQUIRE(parsedPacket);

		checkPacket(parsedPacket.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		checkPacket(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 28,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 3);

		checkPacket(packet.get());

		/* Keep adding RTP packets to the cloned packet. */

		REQUIRE(packet->AddReceivedRtpPacket(1004, 1000000004000));

		const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

		REQUIRE(packet->GetLength() == 28);
		REQUIRE(packet->HasPadding() == true);
		REQUIRE(packet->GetPaddingLength() == 2);
		REQUIRE(packet->GetPacketStatusCount() == 5);
		REQUIRE(receivedRtpPackets.size() == 4);
		REQUIRE(receivedRtpPackets[3].sequenceNumber == 1004);
		REQUIRE(receivedRtpPackets[3].deltaTicks == 4);
	}

	SECTION("TransportWideCcPacket::Factory() without receive deltas succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 5
			0xAF, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Base sequence number: 1, Packet status count: 13
			0x00, 0x01, 0x00, 0x0D,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// One Bit Status Vector Chunk: 1100001111001(0)
			0xB0, 0xF2,
			// Padding: 2 bytes
			0x00, 0x02
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ false)
		};

		packet->SetBase(1, 1000);

		REQUIRE(packet->AddReceivedRtpPacket(1, 1000));
		REQUIRE(packet->AddReceivedRtpPacket(2, 2000));
		REQUIRE(packet->AddReceivedRtpPacket(7, 7000));
		REQUIRE(packet->AddReceivedRtpPacket(8, 8000));
		REQUIRE(packet->AddReceivedRtpPacket(9, 9000));
		REQUIRE(packet->AddReceivedRtpPacket(10, 10000));
		REQUIRE(packet->AddReceivedRtpPacket(13, 13000));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 2);

		REQUIRE(packet->HasDeltas() == false);
		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION(
	  "TransportWideCcPacket::AddReceivedRtpPacket() returns false if there is no space in the buffer")
	{
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, 24, /*includeDeltas*/ true)
		};

		packet->SetBase(0, 0);

		// 20 bytes of fixed header, 2 bytes of packet chunk and 1 byte of receive
		// delta, padded to 24 bytes.
		REQUIRE(packet->AddReceivedRtpPacket(0, 0));
		// 20 bytes of fixed header, 2 bytes of packet chunk and 2 bytes of receive
		// deltas.
		REQUIRE(packet->AddReceivedRtpPacket(1, 0));
		// It would need 28 bytes.
		REQUIRE(!packet->AddReceivedRtpPacket(2, 0));

		REQUIRE(packet->GetLength() == 24);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		REQUIRE(packet->GetReceivedRtpPackets().size() == 2);
	}

	SECTION("TransportWideCcPacket::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, 19, /*includeDeltas*/ true),
		  MediaSoupTypeError);
	}

	/* Feedback packets built from received RTP packets. */

	// Generate receive timestamps for the given sequence numbers, each one being
	// `defaultDeltaUs` after the previous sequence number.
	const auto generateReceiveTimestamps =
	  [](const std::vector<uint16_t>& sequenceNumbers, int64_t defaultDeltaUs) -> std::vector<int64_t>
	{
		uint16_t lastSequenceNumber = sequenceNumbers[0];
		int64_t timeUs{ 0 };
		std::vector<int64_t> timestampsUs;

		for (const uint16_t sequenceNumber : sequenceNumbers)
		{
			if (sequenceNumber < lastSequenceNumber)
			{
				timeUs += 0x10000 * defaultDeltaUs;
			}

			lastSequenceNumber = sequenceNumber;

			timestampsUs.push_back(timeUs + (lastSequenceNumber * defaultDeltaUs));
		}

		return timestampsUs;
	};

	// Build a packet with the given received RTP packets and check its length,
	// its sequence numbers and its receive deltas, before and after parsing it.
	const auto testFeedback = [](
	                            const std::vector<uint16_t>& receivedSequenceNumbers,
	                            const std::vector<int64_t>& receivedTimestampsUs,
	                            bool includeDeltas,
	                            std::optional<size_t> expectedSize) -> void
	{
		REQUIRE(receivedSequenceNumbers.size() == receivedTimestampsUs.size());

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), includeDeltas)
		};
		std::vector<int64_t> expectedDeltasUs;

		packet->SetBase(receivedSequenceNumbers[0], receivedTimestampsUs[0]);

		REQUIRE(packet->AddReceivedRtpPacket(receivedSequenceNumbers[0], receivedTimestampsUs[0]));

		// The first delta is not the difference between two receive times but a
		// compensation for the lower precision of the reference time, so it is
		// the opposite of what GetBaseDeltaUs() returns for the first receive
		// time.
		expectedDeltasUs.push_back(-packet->GetBaseDeltaUs(receivedTimestampsUs[0]));

		for (size_t idx{ 1 }; idx < receivedSequenceNumbers.size(); ++idx)
		{
			REQUIRE(packet->AddReceivedRtpPacket(receivedSequenceNumbers[idx], receivedTimestampsUs[idx]));

			expectedDeltasUs.push_back(receivedTimestampsUs[idx] - receivedTimestampsUs[idx - 1]);
		}

		const auto verify = [&receivedSequenceNumbers, &expectedDeltasUs, includeDeltas, expectedSize](
		                      const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> void
		{
			if (expectedSize.has_value())
			{
				REQUIRE(packet->GetLength() == Utils::Byte::PadTo4Bytes(expectedSize.value()));
			}

			const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

			REQUIRE(receivedRtpPackets.size() == receivedSequenceNumbers.size());

			for (size_t idx{ 0 }; idx < receivedRtpPackets.size(); ++idx)
			{
				REQUIRE(receivedRtpPackets[idx].sequenceNumber == receivedSequenceNumbers[idx]);

				if (includeDeltas)
				{
					REQUIRE(
					  receivedRtpPackets[idx].deltaTicks * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs ==
					  expectedDeltasUs[idx]);
				}
			}
		};

		verify(packet.get());

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
		};

		REQUIRE(parsedPacket);
		REQUIRE(parsedPacket->HasDeltas() == includeDeltas);

		verify(parsedPacket.get());
	};

	SECTION("one bit vector")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 1, 2, 7, 8, 9, 10, 13 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * 4),
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (receivedSequenceNumbers.size() * SmallDeltaSize));
	}

	SECTION("one bit vector without receive deltas")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 1, 2, 7, 8, 9, 10, 13 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * 4),
		  /*includeDeltas*/ false,
		  HeaderSize + StatusChunkSize);
	}

	SECTION("full one bit vector")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 1, 2, 7, 8, 9, 10, 13, 14 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * 4),
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (receivedSequenceNumbers.size() * SmallDeltaSize));
	}

	SECTION("one bit vector with wrap around of received sequence numbers")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 0xFFFD, 0xFFFE, 0xFFFF, 0, 1, 2 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * 4),
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (receivedSequenceNumbers.size() * SmallDeltaSize));
	}

	SECTION("one bit vector with wrap around of missing sequence numbers")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 0xFFFD, 0xFFFE, 1, 2 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * 4),
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (receivedSequenceNumbers.size() * SmallDeltaSize));
	}

	SECTION("two bit vector")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 1, 2, 6, 7 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, DeltaLimitUs + RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs),
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (receivedSequenceNumbers.size() * LargeDeltaSize));
	}

	SECTION("full two bit vector")
	{
		const std::vector<uint16_t> receivedSequenceNumbers{ 1, 2, 6, 7, 8 };

		testFeedback(
		  receivedSequenceNumbers,
		  generateReceiveTimestamps(
		    receivedSequenceNumbers, DeltaLimitUs + RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs),
		  /*includeDeltas*/ true,
		  HeaderSize + (2 * StatusChunkSize) + (receivedSequenceNumbers.size() * LargeDeltaSize));
	}

	SECTION("large and negative deltas")
	{
		testFeedback(
		  { 1, 2, 6, 7, 8 },
		  { 2000, 1000, 4000, 3000, 3000 + (RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs * (1 << 8)) },
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + (3 * LargeDeltaSize) + SmallDeltaSize);
	}

	SECTION("max run length")
	{
		// Expected chunks created:
		// - One Bit Status Vector Chunk (1 x received + 13 x not received).
		// - Run Length Chunk of max length for not received symbol.
		// - Run Length Chunk of length 1 for received symbol.
		static constexpr uint16_t PacketCount{ (1 << 13) - 1 + 14 };

		testFeedback(
		  { 0, PacketCount },
		  { 1000, 2000 },
		  /*includeDeltas*/ true,
		  HeaderSize + (3 * StatusChunkSize) + (2 * SmallDeltaSize));
	}

	SECTION("min run length")
	{
		// Expected chunks created:
		// - One Bit Status Vector Chunk (1 x received + 13 x not received).
		// - Run Length Chunk of length 15 for not received symbol.
		// - Run Length Chunk of length 1 for received symbol.
		testFeedback(
		  { 0, (14 * 2) + 1 },
		  { 1000, 2000 },
		  /*includeDeltas*/ true,
		  HeaderSize + (3 * StatusChunkSize) + (2 * SmallDeltaSize));
	}

	SECTION("one bit vector to two bit vector")
	{
		static constexpr uint16_t TwoBitVectorCapacity{ 7 };

		testFeedback(
		  { 0, TwoBitVectorCapacity - 1 },
		  { 0, DeltaLimitUs + RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs },
		  /*includeDeltas*/ true,
		  HeaderSize + StatusChunkSize + SmallDeltaSize + LargeDeltaSize);
	}

	SECTION("one bit vector to two bit vector, simple split")
	{
		static constexpr uint16_t TwoBitVectorCapacity{ 7 };

		testFeedback(
		  { 0, TwoBitVectorCapacity },
		  { 0, DeltaLimitUs + RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs },
		  /*includeDeltas*/ true,
		  HeaderSize + (2 * StatusChunkSize) + SmallDeltaSize + LargeDeltaSize);
	}

	SECTION("one bit vector to two bit vector, split")
	{
		// With received small delta = S, received large delta = L, use input
		// SSSSSSSSLSSSSSSSSSSSS. This will cause a 1:2 split at the L. After the
		// split there will be two symbols in the symbol vector: SL.
		static constexpr int64_t LargeDeltaUs{ RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs *
		                                       (1 << 8) };
		static constexpr size_t NumPackets{ (3 * 7) + 1 };

		std::vector<uint16_t> receivedSequenceNumbers;
		std::vector<int64_t> receivedTimestampsUs;

		for (size_t idx{ 0 }; idx < NumPackets; ++idx)
		{
			receivedSequenceNumbers.push_back(static_cast<uint16_t>(idx));
		}

		receivedTimestampsUs.push_back(1000);

		for (size_t idx{ 1 }; idx < NumPackets; ++idx)
		{
			const int64_t deltaUs = (idx == 8) ? LargeDeltaUs : 1000;

			receivedTimestampsUs.push_back(receivedTimestampsUs.back() + deltaUs);
		}

		testFeedback(
		  receivedSequenceNumbers,
		  receivedTimestampsUs,
		  /*includeDeltas*/ true,
		  HeaderSize + (StatusChunkSize * 3) + (SmallDeltaSize * (NumPackets - 1)) + (LargeDeltaSize * 1));
	}

	SECTION("large base time")
	{
		static constexpr int64_t TimestampUs{ int64_t{ 0x7FFFFFFF } *
		                                      RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs };

		testFeedback({ 0 }, { TimestampUs }, /*includeDeltas*/ true, std::nullopt);
	}

	SECTION("aliasing")
	{
		static constexpr int Samples{ 100 };
		static constexpr int64_t TooSmallDeltaUs{ RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs / 3 };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet->SetBase(0, 0);

		for (int idx{ 0 }; idx < Samples; ++idx)
		{
			REQUIRE(packet->AddReceivedRtpPacket(static_cast<uint16_t>(idx), idx * TooSmallDeltaUs));
		}

		int64_t accumulatedDeltaUs{ 0 };
		int numSamples{ 0 };

		for (const auto& receivedRtpPacket : packet->GetReceivedRtpPackets())
		{
			accumulatedDeltaUs +=
			  receivedRtpPacket.deltaTicks * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs;

			const int64_t expectedTimeUs = numSamples * TooSmallDeltaUs;

			++numSamples;

			REQUIRE(
			  std::abs(accumulatedDeltaUs - expectedTimeUs) <=
			  RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs / 2);
		}

		REQUIRE(numSamples == Samples);
	}

	SECTION("limits")
	{
		// A sequence number exactly half the sequence number space after the last
		// added one is not considered higher than it.
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet->SetBase(0, 0);

		REQUIRE(packet->AddReceivedRtpPacket(0x0, 0));
		REQUIRE(!packet->AddReceivedRtpPacket(0x8000, 1000));

		// Sequence number more than half the sequence number space after the last
		// added one.
		packet.reset(
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true));

		packet->SetBase(0, 0);

		REQUIRE(packet->AddReceivedRtpPacket(0x0, 0));
		REQUIRE(!packet->AddReceivedRtpPacket(0x8000 + 1, 1000));

		// Packet status count max 0xFFFF. Each RTP packet is less than half the
		// sequence number space after the previous one.
		packet.reset(
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true));

		packet->SetBase(0, 0);

		REQUIRE(packet->AddReceivedRtpPacket(0x0, 0));
		REQUIRE(packet->AddReceivedRtpPacket(0x7FFF, 1000));
		REQUIRE(packet->AddReceivedRtpPacket(0xFFFE, 2000));
		REQUIRE(!packet->AddReceivedRtpPacket(0xFFFF, 3000));

		// Too large delta.
		static constexpr int64_t MaxPositiveTimeDeltaUs{
			std::numeric_limits<int16_t>::max() * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs
		};

		packet.reset(
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true));

		packet->SetBase(0, 0);

		REQUIRE(!packet->AddReceivedRtpPacket(
		  1, MaxPositiveTimeDeltaUs + RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs));
		REQUIRE(packet->AddReceivedRtpPacket(1, MaxPositiveTimeDeltaUs));

		// Too large negative delta.
		static constexpr int64_t MaxNegativeTimeDeltaUs{
			std::numeric_limits<int16_t>::min() * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs
		};
		// Use a larger base time to avoid the base time plus the negative delta
		// to be negative.
		static constexpr int64_t BaseTimeUs{ 1000000000000 };

		packet.reset(
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(
		    rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true));

		packet->SetBase(0, BaseTimeUs);

		REQUIRE(!packet->AddReceivedRtpPacket(
		  1, BaseTimeUs + MaxNegativeTimeDeltaUs - RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs));
		REQUIRE(packet->AddReceivedRtpPacket(1, BaseTimeUs + MaxNegativeTimeDeltaUs));
	}

	SECTION("base time is consistent across multiple packets")
	{
		static constexpr int64_t MaxBaseTimeUs{ BaseTimeWrapPeriodUs - BaseTimeTickUs };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet1{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet1->SetBase(0, MaxBaseTimeUs);

		REQUIRE(packet1->AddReceivedRtpPacket(0, MaxBaseTimeUs));

		// Parse it to simulate sending it over the wire.
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket1{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet1->GetBuffer(), packet1->GetLength())
		};

		REQUIRE(parsedPacket1);

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet2{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer), /*includeDeltas*/ true)
		};

		packet2->SetBase(1, MaxBaseTimeUs + BaseTimeTickUs);

		REQUIRE(packet2->AddReceivedRtpPacket(1, MaxBaseTimeUs + BaseTimeTickUs));

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket2{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet2->GetBuffer(), packet2->GetLength())
		};

		REQUIRE(parsedPacket2);
		REQUIRE(parsedPacket2->GetBaseDeltaUs(parsedPacket1->GetBaseTimeUs()) == BaseTimeTickUs);
	}

	SECTION("supports maximum number of negative deltas")
	{
		static constexpr int64_t MinDeltaUs{ RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs *
		                                     std::numeric_limits<int16_t>::min() };

		// Buffer with space for the maximum packet length.
		std::vector<uint8_t> buffer((1 << 16) * 4);

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  buffer.data(), buffer.size(), /*includeDeltas*/ true)
		};

		// Use a large base time to avoid hitting zero while filling the packet,
		// but a multiple of the wrap period so the Reference time field is 0.
		int64_t timestampUs = 1000 * BaseTimeWrapPeriodUs;
		uint16_t numReceivedRtpPackets{ 0 };

		packet->SetBase(0, timestampUs);

		timestampUs += MinDeltaUs;

		while (packet->AddReceivedRtpPacket(++numReceivedRtpPackets, timestampUs))
		{
			REQUIRE(timestampUs >= -MinDeltaUs);

			timestampUs += MinDeltaUs;
		}

		// Subtract the last RTP packet, which failed to be added.
		--numReceivedRtpPackets;

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
		};

		REQUIRE(parsedPacket);

		const auto receivedRtpPackets = parsedPacket->GetReceivedRtpPackets();

		REQUIRE(receivedRtpPackets.size() == numReceivedRtpPackets);

		for (const auto& receivedRtpPacket : receivedRtpPackets)
		{
			REQUIRE(
			  receivedRtpPacket.deltaTicks * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs ==
			  MinDeltaUs);
		}

		REQUIRE(parsedPacket->GetBaseTimeUs() >= -MinDeltaUs * numReceivedRtpPackets);
	}

	SECTION("padding")
	{
		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// V=2, P=1, FMT=15, PT:205, Length: 5
			0xAF, 0xCD, 0x00, 0x05,
			// SSRC of packet sender: 0
			0x00, 0x00, 0x00, 0x00,
			// SSRC of media source: 0
			0x00, 0x00, 0x00, 0x00,
			// Base sequence number: 0, Packet status count: 1
			0x00, 0x00, 0x00, 0x01,
			// Reference time: 0, Feedback packet count: 0
			0x00, 0x00, 0x00, 0x00,
			// Run Length Chunk: symbol 1, run length 1
			0x20, 0x01,
			// Receive delta: 0
			0x00,
			// Padding: 1 byte
			0x01
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet->SetBase(0, 0);

		REQUIRE(packet->AddReceivedRtpPacket(0, 0));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::NEW_RTCP::Packet::PacketType::RTPFB,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 1);

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("correctly splits vector chunks")
	{
		static constexpr int OneBitVectorCapacity{ 14 };
		static constexpr int64_t LargeTimeDeltaUs{ RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs *
		                                           (1 << 8) };

		// A number of small deltas followed by a large delta must result in a
		// correct split into multiple chunks.
		for (int deltas{ 0 }; deltas <= OneBitVectorCapacity + 1; ++deltas)
		{
			std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
				RTC::NEW_RTCP::TransportWideCcPacket::Factory(
				  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
			};

			packet->SetBase(0, 0);

			for (int idx{ 0 }; idx < deltas; ++idx)
			{
				REQUIRE(packet->AddReceivedRtpPacket(static_cast<uint16_t>(idx), idx * 1000));
			}

			REQUIRE(packet->AddReceivedRtpPacket(
			  static_cast<uint16_t>(deltas), (deltas * 1000) + LargeTimeDeltaUs));

			std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
				RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
			};

			REQUIRE(parsedPacket);
			REQUIRE(parsedPacket->GetReceivedRtpPackets().size() == static_cast<size_t>(deltas + 1));
		}
	}

	SECTION("cloned packet is equal to the original one")
	{
		static constexpr int Samples{ 100 };
		static constexpr uint16_t BaseSequenceNumber{ 7531 };
		static constexpr int64_t BaseTimestampUs{ 123456789 };
		static constexpr uint8_t FeedbackPacketCount{ 90 };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet->SetBase(BaseSequenceNumber, BaseTimestampUs);
		packet->SetFeedbackPacketCount(FeedbackPacketCount);

		for (int idx{ 0 }; idx < Samples; ++idx)
		{
			REQUIRE(packet->AddReceivedRtpPacket(
			  static_cast<uint16_t>(BaseSequenceNumber + idx),
			  BaseTimestampUs + (idx * RTC::NEW_RTCP::TransportWideCcPacket::DeltaTickUs)));
		}

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> clonedPacket{ packet->Clone(
			rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)) };

		REQUIRE(
		  helpers::areBuffersEqual(
		    clonedPacket->GetBuffer(), clonedPacket->GetLength(), packet->GetBuffer(), packet->GetLength()) ==
		  true);

		// Both packets keep building the same way.
		REQUIRE(
		  packet->AddReceivedRtpPacket(BaseSequenceNumber + Samples + 10, BaseTimestampUs + 1000000));
		REQUIRE(clonedPacket->AddReceivedRtpPacket(
		  BaseSequenceNumber + Samples + 10, BaseTimestampUs + 1000000));

		REQUIRE(
		  helpers::areBuffersEqual(
		    clonedPacket->GetBuffer(), clonedPacket->GetLength(), packet->GetBuffer(), packet->GetLength()) ==
		  true);
	}

	SECTION("reports missing RTP packets")
	{
		static constexpr uint16_t BaseSequenceNumber{ 1000 };
		static constexpr int64_t BaseTimestampUs{ 10000 };
		static constexpr uint8_t FeedbackPacketCount{ 90 };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};

		packet->SetBase(BaseSequenceNumber, BaseTimestampUs);
		packet->SetFeedbackPacketCount(FeedbackPacketCount);

		REQUIRE(packet->AddReceivedRtpPacket(BaseSequenceNumber + 0, BaseTimestampUs));
		// RTP packet losses indicated by a jump in the sequence number.
		REQUIRE(packet->AddReceivedRtpPacket(BaseSequenceNumber + 3, BaseTimestampUs + 2000));

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
		};

		REQUIRE(parsedPacket);

		const auto rtpPacketStatuses = getRtpPacketStatuses(parsedPacket.get());

		REQUIRE(rtpPacketStatuses.size() == 4);
		REQUIRE(rtpPacketStatuses[0].sequenceNumber == BaseSequenceNumber + 0);
		REQUIRE(rtpPacketStatuses[0].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[1].sequenceNumber == BaseSequenceNumber + 1);
		REQUIRE(!rtpPacketStatuses[1].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[2].sequenceNumber == BaseSequenceNumber + 2);
		REQUIRE(!rtpPacketStatuses[2].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[3].sequenceNumber == BaseSequenceNumber + 3);
		REQUIRE(rtpPacketStatuses[3].deltaSinceBaseUs.has_value());
	}

	SECTION("reports missing RTP packets without receive deltas")
	{
		static constexpr uint16_t BaseSequenceNumber{ 1000 };
		static constexpr uint8_t FeedbackPacketCount{ 90 };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ false)
		};

		packet->SetBase(BaseSequenceNumber, 10000);
		packet->SetFeedbackPacketCount(FeedbackPacketCount);

		REQUIRE(packet->AddReceivedRtpPacket(BaseSequenceNumber + 0, 0));
		// RTP packet losses indicated by a jump in the sequence number.
		REQUIRE(packet->AddReceivedRtpPacket(BaseSequenceNumber + 3, 0));

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
		};

		REQUIRE(parsedPacket);
		REQUIRE(parsedPacket->HasDeltas() == false);

		const auto rtpPacketStatuses = getRtpPacketStatuses(parsedPacket.get());

		REQUIRE(rtpPacketStatuses.size() == 4);
		REQUIRE(rtpPacketStatuses[0].sequenceNumber == BaseSequenceNumber + 0);
		REQUIRE(rtpPacketStatuses[0].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[1].sequenceNumber == BaseSequenceNumber + 1);
		REQUIRE(!rtpPacketStatuses[1].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[2].sequenceNumber == BaseSequenceNumber + 2);
		REQUIRE(!rtpPacketStatuses[2].deltaSinceBaseUs.has_value());
		REQUIRE(rtpPacketStatuses[3].sequenceNumber == BaseSequenceNumber + 3);
		REQUIRE(rtpPacketStatuses[3].deltaSinceBaseUs.has_value());
	}

	/* Feedback packets built from a list of inputs whose first one sets the base. */

	/**
	 * Received RTP packet given to the packet.
	 */
	struct Input
	{
		/**
		 * Transport-wide sequence number.
		 */
		uint16_t sequenceNumber;
		/**
		 * Receive time (in microseconds).
		 */
		int64_t timestampUs;
	};

	// Build a packet whose base is set with the first input (one sequence number
	// after it) and with the rest of the inputs as received RTP packets.
	const auto buildPacket = [](
	                           const std::vector<Input>& inputs,
	                           uint8_t feedbackPacketCount,
	                           uint8_t* buffer,
	                           size_t bufferLength) -> RTC::NEW_RTCP::TransportWideCcPacket*
	{
		auto* packet =
		  RTC::NEW_RTCP::TransportWideCcPacket::Factory(buffer, bufferLength, /*includeDeltas*/ true);

		packet->SetSenderSsrc(1111);
		packet->SetMediaSsrc(2222);
		packet->SetFeedbackPacketCount(feedbackPacketCount);
		packet->SetBase(
		  static_cast<uint16_t>(inputs.front().sequenceNumber + 1), inputs.front().timestampUs);

		for (size_t idx{ 1 }; idx < inputs.size(); ++idx)
		{
			const auto& input = inputs[idx];

			REQUIRE(packet->AddReceivedRtpPacket(input.sequenceNumber, input.timestampUs));
		}

		return packet;
	};

	// Check that every input is reported with its receive time and that every
	// sequence number between them is reported as not received.
	const auto verifyInputs = [&getRtpPacketStatuses](
	                            const std::vector<Input>& inputs,
	                            const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> void
	{
		// The reference time only carries whole base time ticks and wraps around,
		// so the reconstructed times sit in a frame shifted a constant amount from
		// the given ones. The shift comes from the timestamp the base was set
		// with, which is the first input.
		const int64_t baseTimeTicks       = inputs.front().timestampUs / BaseTimeTickUs;
		const int64_t maskedBaseTimeTicks = baseTimeTicks & 0xFFFFFF;
		// The reference time is a 24 bits signed integer.
		const int64_t referenceTime =
		  maskedBaseTimeTicks >= (1 << 23) ? maskedBaseTimeTicks - (1 << 24) : maskedBaseTimeTicks;
		const int64_t frameOffsetUs =
		  BaseTimeWrapPeriodUs + ((referenceTime - baseTimeTicks) * BaseTimeTickUs);
		const auto rtpPacketStatuses = getRtpPacketStatuses(packet);

		size_t statusIdx{ 0 };
		Input lastInput = inputs.front();

		for (size_t inputIdx{ 1 }; inputIdx < inputs.size(); ++inputIdx)
		{
			const auto& input = inputs[inputIdx];
			const auto missingRtpPackets =
			  static_cast<uint16_t>(input.sequenceNumber - lastInput.sequenceNumber - 1);

			// All missing RTP packets must be reported as not received.
			for (uint16_t idx{ 0 }; idx < missingRtpPackets; ++idx)
			{
				REQUIRE(statusIdx < rtpPacketStatuses.size());

				const auto& rtpPacketStatus = rtpPacketStatuses[statusIdx];

				REQUIRE(
				  rtpPacketStatus.sequenceNumber == static_cast<uint16_t>(lastInput.sequenceNumber + idx + 1));
				REQUIRE(!rtpPacketStatus.deltaSinceBaseUs.has_value());

				++statusIdx;
			}

			REQUIRE(statusIdx < rtpPacketStatuses.size());

			const auto& rtpPacketStatus = rtpPacketStatuses[statusIdx];

			REQUIRE(rtpPacketStatus.sequenceNumber == input.sequenceNumber);
			// Receive deltas carry the receive times with no loss as long as the
			// given ones are multiples of a delta tick.
			REQUIRE(
			  rtpPacketStatus.deltaSinceBaseUs ==
			  input.timestampUs + frameOffsetUs - packet->GetBaseTimeUs());

			++statusIdx;

			lastInput = input;
		}

		REQUIRE(statusIdx == rtpPacketStatuses.size());
	};

	// Check that the packet parses into one that tells the same and that is
	// serialized into the same bytes.
	const auto verifyParsedPacket = [&verifyInputs, &getNumberOfLostRtpPackets](
	                                  const std::vector<Input>& inputs,
	                                  const RTC::NEW_RTCP::TransportWideCcPacket* packet) -> void
	{
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> parsedPacket{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(packet->GetBuffer(), packet->GetLength())
		};

		REQUIRE(parsedPacket);
		REQUIRE(parsedPacket->GetBaseSequenceNumber() == packet->GetBaseSequenceNumber());
		REQUIRE(parsedPacket->GetPacketStatusCount() == packet->GetPacketStatusCount());
		REQUIRE(parsedPacket->GetFeedbackPacketCount() == packet->GetFeedbackPacketCount());
		REQUIRE(getNumberOfLostRtpPackets(parsedPacket.get()) == getNumberOfLostRtpPackets(packet));
		// The reference time survives the round trip, so a parsed packet tells
		// the very same receive times as the one it was serialized from.
		REQUIRE(parsedPacket->GetBaseTimeUs() == packet->GetBaseTimeUs());

		verifyInputs(inputs, parsedPacket.get());

		parsedPacket->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		REQUIRE(
		  helpers::areBuffersEqual(
		    parsedPacket->GetBuffer(), parsedPacket->GetLength(), packet->GetBuffer(), packet->GetLength()) ==
		  true);
	};

	SECTION("small delta run length chunk and single large delta status packet")
	{
		// clang-format off
		const std::vector<Input> inputs =
		{
			{ .sequenceNumber = 999, .timestampUs = 1000000000000 }, // Pre base.
			{ .sequenceNumber = 1000, .timestampUs = 1000000000000 }, // Base.
			{ .sequenceNumber = 1001, .timestampUs = 1000000001000 },
			{ .sequenceNumber = 1002, .timestampUs = 1000000012000 },
			{ .sequenceNumber = 1003, .timestampUs = 1000000015000 },
			{ .sequenceNumber = 1004, .timestampUs = 1000000017000 },
			{ .sequenceNumber = 1005, .timestampUs = 1000000018000 },
			{ .sequenceNumber = 1006, .timestampUs = 1000000018000 },
			{ .sequenceNumber = 1007, .timestampUs = 1000000018000 },
			{ .sequenceNumber = 1008, .timestampUs = 1000000018000 },
			{ .sequenceNumber = 1009, .timestampUs = 1000000019000 },
			{ .sequenceNumber = 1010, .timestampUs = 1000000010000 },
			{ .sequenceNumber = 1011, .timestampUs = 1000000011000 },
			{ .sequenceNumber = 1012, .timestampUs = 1000000011000 },
			{ .sequenceNumber = 1013, .timestampUs = 1000000013000 },
			// Greater sequence number but older timestamp.
			{ .sequenceNumber = 1014, .timestampUs = 1000000013000 - 128000 },
			{ .sequenceNumber = 1015, .timestampUs = 1000000015000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{ buildPacket(
			inputs, 1, rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		verifyInputs(inputs, packet.get());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 16);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(getNumberOfLostRtpPackets(packet.get()) == 0);
		REQUIRE(packet->GetReceivedRtpPackets().back().sequenceNumber == 1015);

		verifyParsedPacket(inputs, packet.get());
	}

	SECTION("not received run length chunk and large delta")
	{
		// clang-format off
		const std::vector<Input> inputs =
		{
			{ .sequenceNumber = 999, .timestampUs = 1000000000000 }, // Pre base.
			{ .sequenceNumber = 1000, .timestampUs = 1000000000000 }, // Base.
			{ .sequenceNumber = 1050, .timestampUs = 1000000216000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{ buildPacket(
			inputs, 10, rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		verifyInputs(inputs, packet.get());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 51);
		REQUIRE(packet->GetFeedbackPacketCount() == 10);
		REQUIRE(getNumberOfLostRtpPackets(packet.get()) > 0);
		REQUIRE(packet->GetReceivedRtpPackets().back().sequenceNumber == 1050);

		verifyParsedPacket(inputs, packet.get());
	}

	SECTION("mixed chunks")
	{
		// clang-format off
		const std::vector<Input> inputs =
		{
			{ .sequenceNumber = 999, .timestampUs = 1000000000000 }, // Pre base.
			{ .sequenceNumber = 1000, .timestampUs = 1000000000000 }, // Base.
			{ .sequenceNumber = 1001, .timestampUs = 1000000100000 },
			{ .sequenceNumber = 1002, .timestampUs = 1000000200000 },
			{ .sequenceNumber = 1015, .timestampUs = 1000000300000 },
			{ .sequenceNumber = 1016, .timestampUs = 1000000400000 },
			{ .sequenceNumber = 1017, .timestampUs = 1000000500000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{ buildPacket(
			inputs, 1, rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		verifyInputs(inputs, packet.get());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 18);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(getNumberOfLostRtpPackets(packet.get()) > 0);
		REQUIRE(packet->GetReceivedRtpPackets().back().sequenceNumber == 1017);

		verifyParsedPacket(inputs, packet.get());
	}

	SECTION("incomplete two bit vector chunk")
	{
		// clang-format off
		const std::vector<Input> inputs =
		{
			{ .sequenceNumber = 999, .timestampUs = 1000000000000 }, // Pre base.
			{ .sequenceNumber = 1000, .timestampUs = 1000000100000 }, // Base.
			{ .sequenceNumber = 1001, .timestampUs = 1000000700000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{ buildPacket(
			inputs, 1, rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		verifyInputs(inputs, packet.get());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(getNumberOfLostRtpPackets(packet.get()) == 0);
		REQUIRE(packet->GetReceivedRtpPackets().back().sequenceNumber == 1001);

		verifyParsedPacket(inputs, packet.get());
	}

	SECTION("two sequential packets")
	{
		// clang-format off
		const std::vector<Input> inputs =
		{
			{ .sequenceNumber = 999, .timestampUs = 1000000000000 }, // Pre base.
			{ .sequenceNumber = 1000, .timestampUs = 1000000000000 }, // Base.
			{ .sequenceNumber = 1001, .timestampUs = 1000000003000 },
			{ .sequenceNumber = 1002, .timestampUs = 1000000003000 },
			{ .sequenceNumber = 1003, .timestampUs = 1000000003000 },
			{ .sequenceNumber = 1004, .timestampUs = 1000000004000 },
			{ .sequenceNumber = 1005, .timestampUs = 1000000005000 },
			{ .sequenceNumber = 1006, .timestampUs = 1000000005000 },
			{ .sequenceNumber = 1007, .timestampUs = 1000000007000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{ buildPacket(
			inputs, 1, rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		verifyInputs(inputs, packet.get());

		REQUIRE(packet->GetBaseSequenceNumber() == 1000);
		REQUIRE(packet->GetPacketStatusCount() == 8);
		REQUIRE(packet->GetFeedbackPacketCount() == 1);
		REQUIRE(getNumberOfLostRtpPackets(packet.get()) == 0);
		REQUIRE(packet->GetReceivedRtpPackets().back().sequenceNumber == 1007);

		verifyParsedPacket(inputs, packet.get());

		// The second packet starts right after the last RTP packet of the first
		// one.
		// clang-format off
		const std::vector<Input> inputs2 =
		{
			inputs.back(),
			{ .sequenceNumber = 1008, .timestampUs = 1000000008000 },
			{ .sequenceNumber = 1009, .timestampUs = 1000000009000 },
			{ .sequenceNumber = 1010, .timestampUs = 1000000010000 },
			{ .sequenceNumber = 1011, .timestampUs = 1000000010000 },
			{ .sequenceNumber = 1012, .timestampUs = 1000000010000 },
			{ .sequenceNumber = 1013, .timestampUs = 1000000014000 },
			{ .sequenceNumber = 1014, .timestampUs = 1000000014000 }
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet2{ buildPacket(
			inputs2, 2, rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)) };

		verifyInputs(inputs2, packet2.get());

		REQUIRE(packet2->GetBaseSequenceNumber() == 1008);
		REQUIRE(packet2->GetPacketStatusCount() == 7);
		REQUIRE(packet2->GetFeedbackPacketCount() == 2);
		REQUIRE(getNumberOfLostRtpPackets(packet2.get()) == 0);
		REQUIRE(packet2->GetReceivedRtpPackets().back().sequenceNumber == 1014);

		verifyParsedPacket(inputs2, packet2.get());
	}

	SECTION("parse packet generated by Chrome")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			0x8F, 0xCD, 0x00, 0x05,
			0xFA, 0x17, 0xFA, 0x17,
			0x39, 0xE9, 0x42, 0x38,
			0x00, 0x01, 0x00, 0x02,
			0xBD, 0x57, 0xAA, 0x00,
			0x20, 0x02, 0x8C, 0x44
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
			RTC::NEW_RTCP::TransportWideCcPacket::Parse(buffer, sizeof(buffer))
		};

		REQUIRE(packet);
		REQUIRE(packet->GetLength() == sizeof(buffer));
		REQUIRE(packet->GetBaseSequenceNumber() == 1);
		REQUIRE(packet->GetPacketStatusCount() == 2);
		// The Reference time field is a 24 bits signed integer.
		REQUIRE(
		  packet->GetBaseTimeUs() ==
		  BaseTimeWrapPeriodUs + (static_cast<int64_t>(-4368470) * BaseTimeTickUs));
		REQUIRE(packet->GetFeedbackPacketCount() == 0);

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			0x8F, 0xCD, 0x00, 0x05,
			0xFA, 0x17, 0xFA, 0x17,
			0x39, 0xE9, 0x42, 0x38,
			0x00, 0x01, 0x00, 0x02,
			0xBD, 0x57, 0xAA, 0x00,
			0x20, 0x02, 0x8C, 0x44
		};
		// clang-format on

		REQUIRE(
		  helpers::areBuffersEqual(
		    packet->GetBuffer(), packet->GetLength(), expectedBuffer, sizeof(expectedBuffer)) == true);
	}

	SECTION("parse packets generated by Chrome with libwebrtc as a reference")
	{
		/**
		 * Values of a packet parsed with libwebrtc, and the packet.
		 */
		struct FeedbackPacketMeta
		{
			/**
			 * Reference time (in microseconds).
			 */
			int64_t baseTimeUs;
			/**
			 * Base sequence number.
			 */
			uint16_t baseSequence;
			/**
			 * Packet status count.
			 */
			uint16_t packetStatusCount;
			/**
			 * Receive deltas (in multiples of the delta tick).
			 */
			std::vector<int16_t> deltas;
			/**
			 * Packet.
			 */
			std::vector<uint8_t> buffer;
		};

		// clang-format off
		const std::vector<FeedbackPacketMeta> feedbackPacketsMeta =
		{
			{
				.baseTimeUs        = 1076014080000,
				.baseSequence      = 13,
				.packetStatusCount = 1,
				.deltas            = { 228 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x05, 0xFA, 0x17, 0xFA, 0x17,
				                       0x00, 0x00, 0x04, 0xD2, 0x00, 0x0D, 0x00, 0x01,
				                       0x00, 0x8A, 0xB0, 0x00, 0x20, 0x01, 0xE4, 0x01 }
			},
			{
				.baseTimeUs        = 1076014080000,
				.baseSequence      = 14,
				.packetStatusCount = 4,
				.deltas            = { 232, 8, 12, 220 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x1C, 0xB7,
				                       0xDA, 0xF3, 0x00, 0x0E, 0x00, 0x04, 0x00, 0x8A, 0xB0, 0x01,
				                       0x20, 0x04, 0xE8, 0x08, 0x0C, 0xDC, 0x00, 0x02 }
			},
			{
				.baseTimeUs        = 1076014144000,
				.baseSequence      = 18,
				.packetStatusCount = 5,
				.deltas            = { 240, 24, 20, 36, 88 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x1C, 0xB7,
				                       0xDA, 0xF3, 0x00, 0x12, 0x00, 0x05, 0x00, 0x8A, 0xB1, 0x02,
				                       0x20, 0x05, 0xF0, 0x18, 0x14, 0x24, 0x58, 0x01 }
			},
			{
				.baseTimeUs        = 1113285696000,
				.baseSequence      = 2924,
				.packetStatusCount = 22,
				.deltas            = { 12, 20, 20, 0, 40, 0, 0, 16, 0, 4, 0,
				                       8,  0,  8,  0, 8,  0, 8, 0,  4, 0, 16 },
				.buffer            = { 0x8F, 0xCD, 0x00, 0x0A, 0xFA, 0x17, 0xFA, 0x17, 0x06,
				                       0xF5, 0x11, 0x4C, 0x0B, 0x6C, 0x00, 0x16, 0x09, 0x6D,
				                       0x91, 0xEE, 0x20, 0x16, 0x0C, 0x14, 0x14, 0x00, 0x28,
				                       0x00, 0x00, 0x10, 0x00, 0x04, 0x00, 0x08, 0x00, 0x08,
				                       0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x00, 0x10 }
			},
			{
				.baseTimeUs        = 794159744000,
				.baseSequence      = 1,
				.packetStatusCount = 2,
				.deltas            = { 140, 68 },
				.buffer            = { 0x8F, 0xCD, 0x00, 0x05, 0xFA, 0x17, 0xFA, 0x17,
				                       0x39, 0xE9, 0x42, 0x38, 0x00, 0x01, 0x00, 0x02,
				                       0xBD, 0x57, 0xAA, 0x00, 0x20, 0x02, 0x8C, 0x44 }
			},
			{
				.baseTimeUs        = 1126157504000,
				.baseSequence      = 930,
				.packetStatusCount = 5,
				.deltas            = { 248, 72, 20, 24, 76 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x26, 0x9E,
				                       0x8E, 0x50, 0x03, 0xA2, 0x00, 0x05, 0x0C, 0x7F, 0x33, 0x9F,
				                       0x20, 0x05, 0xF8, 0x48, 0x14, 0x18, 0x4C, 0x01 }
			},
			{
				.baseTimeUs        = 1126157568000,
				.baseSequence      = 921,
				.packetStatusCount = 7,
				.deltas            = { 56, 20, 24, 24, 28, 56, 20 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x07, 0xFA, 0x17, 0xFA, 0x17, 0x33, 0xB0, 0x4A,
				                       0xE8, 0x03, 0x99, 0x00, 0x07, 0x0C, 0x7F, 0x34, 0x9F, 0x20, 0x07,
				                       0x38, 0x14, 0x18, 0x18, 0x1C, 0x38, 0x14, 0x00, 0x00, 0x03 }
			},
			{
				.baseTimeUs        = 1126157568000,
				.baseSequence      = 935,
				.packetStatusCount = 7,
				.deltas            = { 228, 0, 24, 20, 20, 96, 0 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x07, 0xFA, 0x17, 0xFA, 0x17, 0x26, 0x9E, 0x8E,
				                       0x50, 0x03, 0xA7, 0x00, 0x07, 0x0C, 0x7F, 0x34, 0xA0, 0x20, 0x07,
				                       0xE4, 0x00, 0x18, 0x14, 0x14, 0x60, 0x00, 0x00, 0x00, 0x03 }
			},
			{
				.baseTimeUs        = 1126157568000,
				.baseSequence      = 928,
				.packetStatusCount = 5,
				.deltas            = { 252, 44, 84, 24, 0 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x33, 0xB0,
				                       0x4A, 0xE8, 0x03, 0xA0, 0x00, 0x05, 0x0C, 0x7F, 0x34, 0xA0,
				                       0x20, 0x05, 0xFC, 0x2C, 0x54, 0x18, 0x00, 0x01 }
			},
			{
				.baseTimeUs        = 1126157632000,
				.baseSequence      = 942,
				.packetStatusCount = 6,
				.deltas            = { 156, 52, 36, 20, 16, 52 },
				.buffer            = { 0x8F, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x26, 0x9E,
				                       0x8E, 0x50, 0x03, 0xAE, 0x00, 0x06, 0x0C, 0x7F, 0x35, 0xA1,
				                       0x20, 0x06, 0x9C, 0x34, 0x24, 0x14, 0x10, 0x34 }
			},
			{
				.baseTimeUs        = 1126319296000,
				.baseSequence      = 10,
				.packetStatusCount = 7,
				.deltas            = { 100, 8, 8, 12, 4, 4, 12 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x07, 0xFA, 0x17, 0xFA, 0x17, 0x00, 0x00, 0x04,
				                       0xD2, 0x00, 0x0A, 0x00, 0x07, 0x0C, 0x89, 0x13, 0x00, 0x20, 0x07,
				                       0x64, 0x08, 0x08, 0x0C, 0x04, 0x04, 0x0C, 0x00, 0x00, 0x03 }
			},
			{
				.baseTimeUs        = 1126319360000,
				.baseSequence      = 17,
				.packetStatusCount = 2,
				.deltas            = { 176, 72 },
				.buffer            = { 0x8F, 0xCD, 0x00, 0x05, 0xFA, 0x17, 0xFA, 0x17,
				                       0x08, 0xEB, 0x06, 0xD7, 0x00, 0x11, 0x00, 0x02,
				                       0x0C, 0x89, 0x14, 0x01, 0x20, 0x02, 0xB0, 0x48 }
			},
			{
				.baseTimeUs        = 1126319360000,
				.baseSequence      = 17,
				.packetStatusCount = 1,
				.deltas            = { 248 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x05, 0xFA, 0x17, 0xFA, 0x17,
				                       0x20, 0x92, 0x5E, 0xB7, 0x00, 0x11, 0x00, 0x01,
				                       0x0C, 0x89, 0x14, 0x00, 0x20, 0x01, 0xF8, 0x01 }
			},
			{
				.baseTimeUs        = 1126319488000,
				.baseSequence      = 19,
				.packetStatusCount = 4,
				.deltas            = { 16, 0, 16, 0 },
				.buffer            = { 0xAF, 0xCD, 0x00, 0x06, 0xFA, 0x17, 0xFA, 0x17, 0x08, 0xEB,
				                       0x06, 0xD7, 0x00, 0x13, 0x00, 0x04, 0x0C, 0x89, 0x16, 0x02,
				                       0x20, 0x04, 0x10, 0x00, 0x10, 0x00, 0x00, 0x02 }
			}
		};
		// clang-format on

		for (const auto& feedbackPacketMeta : feedbackPacketsMeta)
		{
			std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet{
				RTC::NEW_RTCP::TransportWideCcPacket::Parse(
				  feedbackPacketMeta.buffer.data(), feedbackPacketMeta.buffer.size())
			};

			REQUIRE(packet);
			REQUIRE(packet->GetBaseTimeUs() == feedbackPacketMeta.baseTimeUs);
			REQUIRE(packet->GetBaseSequenceNumber() == feedbackPacketMeta.baseSequence);
			REQUIRE(packet->GetPacketStatusCount() == feedbackPacketMeta.packetStatusCount);

			const auto receivedRtpPackets = packet->GetReceivedRtpPackets();

			REQUIRE(receivedRtpPackets.size() == feedbackPacketMeta.deltas.size());

			for (size_t idx{ 0 }; idx < feedbackPacketMeta.deltas.size(); ++idx)
			{
				REQUIRE(receivedRtpPackets[idx].deltaTicks == feedbackPacketMeta.deltas[idx]);
			}
		}
	}

	SECTION("GetBaseDeltaUs() compensates the wrap around")
	{
		static constexpr int64_t MaxBaseTimeUs{ BaseTimeWrapPeriodUs - BaseTimeTickUs };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet1{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet2{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer), /*includeDeltas*/ true)
		};
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet3{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer), /*includeDeltas*/ true)
		};

		packet1->SetBase(0, MaxBaseTimeUs);
		packet2->SetBase(0, MaxBaseTimeUs + BaseTimeTickUs);
		packet3->SetBase(0, MaxBaseTimeUs + BaseTimeTickUs + BaseTimeTickUs);

		// Reference time fields are -1, 0 and 1.
		REQUIRE(packet1->GetBaseTimeUs() == 1073741760000);
		REQUIRE(packet2->GetBaseTimeUs() == 1073741824000);
		REQUIRE(packet3->GetBaseTimeUs() == 1073741888000);

		REQUIRE(packet1->GetBaseDeltaUs(packet1->GetBaseTimeUs()) == 0);
		REQUIRE(packet2->GetBaseDeltaUs(packet1->GetBaseTimeUs()) == 64000);
		REQUIRE(packet3->GetBaseDeltaUs(packet2->GetBaseTimeUs()) == 64000);
		REQUIRE(packet3->GetBaseDeltaUs(packet1->GetBaseTimeUs()) == 128000);
	}

	SECTION("SetBase() wraps the reference time over the whole period GetBaseDeltaUs() compensates for")
	{
		// Timestamp sitting on the last whole base time tick before the reference
		// time wraps around.
		static constexpr int64_t LastTickUs{ BaseTimeWrapPeriodUs - BaseTimeTickUs };

		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet1{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer), /*includeDeltas*/ true)
		};
		std::unique_ptr<RTC::NEW_RTCP::TransportWideCcPacket> packet2{
			RTC::NEW_RTCP::TransportWideCcPacket::Factory(
			  rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer), /*includeDeltas*/ true)
		};

		packet1->SetBase(1000, LastTickUs);
		packet2->SetBase(1001, LastTickUs + BaseTimeTickUs);

		// Reference time fields are -1 and 0.
		REQUIRE(packet1->GetBaseTimeUs() == BaseTimeWrapPeriodUs - BaseTimeTickUs);
		REQUIRE(packet2->GetBaseTimeUs() == BaseTimeWrapPeriodUs);

		// Both packets are a single tick apart, so the wrap around must be
		// compensated. It would not be if the reference time wrapped sooner than
		// the period GetBaseDeltaUs() knows about.
		REQUIRE(packet2->GetBaseDeltaUs(packet1->GetBaseTimeUs()) == BaseTimeTickUs);
	}
}
