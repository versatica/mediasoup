#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Packet", "[serializable][sctp][packet]")
{
	sctpCommon::ResetBuffers();

	SECTION("alignof() SCTP structs")
	{
		REQUIRE(alignof(RTC::SCTP::Packet::CommonHeader) == 4);
	}

	SECTION("Parse() without Chunks succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Source Port: 10000, Destination Port: 15999
			0x27, 0x10, 0x3E, 0x7F,
			// Verification Tag: 4294967285
			0xFF, 0xFF, 0xFF, 0xF5,
			// Checksum: 5
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Parse(buffer, sizeof(buffer)) };

		// NOTE: Obviously the Checksum CRC32C validation fails since Checksum is
		// totally random.
		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == nullptr);

		/* Serialize it. */

		packet->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == nullptr);

		/* Insert CRC32C checksum. */

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		/* Clone it. */

		packet.reset(packet->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == nullptr);
	}

	SECTION("Parse() with Chunks succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Source Port: 10000, Destination Port: 15999
			0x27, 0x10, 0x3E, 0x7F,
			// Verification Tag: 4294967285
			0xFF, 0xFF, 0xFF, 0xF5,
			// Checksum: 5
			0x00, 0x00, 0x00, 0x05,
			// Chunk 1: Type:0 (DATA), I:1, U:0, B:1, E:1, Length: 18
			0x00, 0b00001011, 0x00, 0x12,
			// TSN: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Stream Identifier S: 0xFF00, Stream Sequence Number n: 0x6677
			0xFF, 0x00, 0x66, 0x77,
			// Payload Protocol Identifier: 0x12341234
			0x12, 0x34, 0x12, 0x34,
			// User Data (2 bytes): 0xABCD, 2 bytes of padding
			0xAB, 0xCD, 0x00, 0x00,
			// Chunk 2: Type:0xEE (UNKNOWN), Flags: 0b00001100, Length: 7
			0xEE, 0b00001100, 0x00, 0x07,
			// Unknown data: 0xAABBCC, 1 byte of padding
			0xAA, 0xBB, 0xCC, 0x00,
			// Chunk 3: Type:5 (HEARTBEAT_ACK), Flags:0b00000000, Length: 10
			// NOTE: Chunk Length field must exclude padding of the last Parameter.
			0x05, 0b00000000, 0x00, 0x0A,
			// Parameter 1: Type:1 (HEARBEAT_INFO), Length: 6
			0x00, 0x01, 0x00, 0x06,
			// Heartbeat Information (2 bytes): 0x1122, 2 bytes of padding
			0x11, 0x22, 0x00, 0x00,
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Parse(buffer, sizeof(buffer)) };

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 52,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() == nullptr);

		const auto* chunk1 = reinterpret_cast<const RTC::SCTP::DataChunk*>(packet->GetChunkAt(0));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == chunk1);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk1->GetI() == true);
		REQUIRE(chunk1->GetU() == false);
		REQUIRE(chunk1->GetB() == true);
		REQUIRE(chunk1->GetE() == true);
		REQUIRE(chunk1->GetTsn() == 0x11223344);
		REQUIRE(chunk1->GetStreamId() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(chunk1->HasUserDataPayload() == true);
		REQUIRE(chunk1->GetUserDataPayloadLength() == 2);
		REQUIRE(chunk1->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk1->GetUserDataPayload()[1] == 0xCD);

		const auto* chunk2 = reinterpret_cast<const RTC::SCTP::UnknownChunk*>(packet->GetChunkAt(1));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>() == chunk2);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*chunkType*/ static_cast<RTC::SCTP::Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk2->HasUnknownValue() == true);
		REQUIRE(chunk2->GetUnknownValueLength() == 3);
		REQUIRE(chunk2->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk2->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk2->GetUnknownValue()[2] == 0xCC);
		// Padding.
		REQUIRE(chunk2->GetUnknownValue()[3] == 0x00);

		const auto* chunk3 = reinterpret_cast<const RTC::SCTP::HeartbeatAckChunk*>(packet->GetChunkAt(2));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() == chunk3);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		// NOLINTNEXTLINE (readability-identifier-naming)
		const auto* parameter3_1 =
		  reinterpret_cast<const RTC::SCTP::HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);

		/* Serialize it. */

		packet->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 52,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == chunk1);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>() == chunk2);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() == chunk3);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() == nullptr);

		chunk1 = reinterpret_cast<const RTC::SCTP::DataChunk*>(packet->GetChunkAt(0));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk1->GetI() == true);
		REQUIRE(chunk1->GetU() == false);
		REQUIRE(chunk1->GetB() == true);
		REQUIRE(chunk1->GetE() == true);
		REQUIRE(chunk1->GetTsn() == 0x11223344);
		REQUIRE(chunk1->GetStreamId() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(chunk1->HasUserDataPayload() == true);
		REQUIRE(chunk1->GetUserDataPayloadLength() == 2);
		REQUIRE(chunk1->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk1->GetUserDataPayload()[1] == 0xCD);

		chunk2 = reinterpret_cast<const RTC::SCTP::UnknownChunk*>(packet->GetChunkAt(1));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*chunkType*/ static_cast<RTC::SCTP::Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk2->HasUnknownValue() == true);
		REQUIRE(chunk2->GetUnknownValueLength() == 3);
		REQUIRE(chunk2->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk2->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk2->GetUnknownValue()[2] == 0xCC);
		// Padding.
		REQUIRE(chunk2->GetUnknownValue()[3] == 0x00);

		chunk3 = reinterpret_cast<const RTC::SCTP::HeartbeatAckChunk*>(packet->GetChunkAt(2));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter3_1 =
		  reinterpret_cast<const RTC::SCTP::HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);

		/* Clone it. */

		packet.reset(packet->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 52,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() == nullptr);

		chunk1 = reinterpret_cast<const RTC::SCTP::DataChunk*>(packet->GetChunkAt(0));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>() == chunk1);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk1->GetI() == true);
		REQUIRE(chunk1->GetU() == false);
		REQUIRE(chunk1->GetB() == true);
		REQUIRE(chunk1->GetE() == true);
		REQUIRE(chunk1->GetTsn() == 0x11223344);
		REQUIRE(chunk1->GetStreamId() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(chunk1->HasUserDataPayload() == true);
		REQUIRE(chunk1->GetUserDataPayloadLength() == 2);
		REQUIRE(chunk1->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk1->GetUserDataPayload()[1] == 0xCD);

		chunk2 = reinterpret_cast<const RTC::SCTP::UnknownChunk*>(packet->GetChunkAt(1));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>() == chunk2);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*chunkType*/ static_cast<RTC::SCTP::Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b00001100,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk2->HasUnknownValue() == true);
		REQUIRE(chunk2->GetUnknownValueLength() == 3);
		REQUIRE(chunk2->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk2->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk2->GetUnknownValue()[2] == 0xCC);
		// Padding.
		REQUIRE(chunk2->GetUnknownValue()[3] == 0x00);

		chunk3 = reinterpret_cast<const RTC::SCTP::HeartbeatAckChunk*>(packet->GetChunkAt(2));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>() == chunk3);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter3_1 =
		  reinterpret_cast<const RTC::SCTP::HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);
	}

	SECTION("Factory() with Chunks succeeds")
	{
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == nullptr);

		/* Modify the Packet and add Chunks. */

		packet->SetSourcePort(1000);
		packet->SetDestinationPort(6000);
		packet->SetVerificationTag(12345678);
		packet->SetChecksum(0);

		// Chunk 1: INIT, length: 20 bytes.
		auto* chunk1 = packet->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		chunk1->SetInitiateTag(87654321);
		chunk1->SetAdvertisedReceiverWindowCredit(12345678);
		chunk1->SetNumberOfOutboundStreams(11100);
		chunk1->SetNumberOfInboundStreams(22200);
		chunk1->SetInitialTsn(14141414);

		// Parameter 1.1: IPV4_ADDRESS, length: 8 bytes.
		// NOLINTNEXTLINE (readability-identifier-naming)
		auto* parameter1_1 = chunk1->BuildParameterInPlace<RTC::SCTP::IPv4AddressParameter>();

		// 192.168.0.3 IPv4 in network order.
		uint8_t ipBuffer[] = { 0xC0, 0xA8, 0x00, 0x03 };

		parameter1_1->SetIPv4Address(ipBuffer);
		parameter1_1->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<RTC::SCTP::IPv4AddressParameter>() == parameter1_1);

		// Parameter 1.2: COOKIE_PRESERVATIVE, length: 8 bytes.
		// NOLINTNEXTLINE (readability-identifier-naming)
		auto* parameter1_2 = chunk1->BuildParameterInPlace<RTC::SCTP::CookiePreservativeParameter>();

		parameter1_2->SetLifeSpanIncrement(987654321);
		parameter1_2->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<RTC::SCTP::CookiePreservativeParameter>() == parameter1_2);

		// Consolidate Chunk 1 after consolidating its Parameters 1.1 and 1.2.
		chunk1->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<RTC::SCTP::IPv4AddressParameter>() == parameter1_1);
		REQUIRE(chunk1->GetFirstParameterOfType<RTC::SCTP::CookiePreservativeParameter>() == parameter1_2);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == chunk1);

		// Chunk 2: HEARTBEAT_REQUEST, length: 4 bytes.
		auto* chunk2 = packet->BuildChunkInPlace<RTC::SCTP::HeartbeatRequestChunk>();

		// Parameter 2.1: HEARTBEAT_INFO, length: 4 bytes.
		// NOLINTNEXTLINE (readability-identifier-naming)
		auto* parameter2_1 = chunk2->BuildParameterInPlace<RTC::SCTP::HeartbeatInfoParameter>();

		// Parameter 2.1: Add 3 bytes of info + 1 byte of padding.
		parameter2_1->SetInfo(sctpCommon::DataBuffer, 3);
		parameter2_1->Consolidate();

		REQUIRE(chunk2->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>() == parameter2_1);

		std::memset(sctpCommon::DataBuffer, 0xFF, 3);

		// Consolidate the Chunk after consolidating its Parameters.
		chunk2->Consolidate();

		REQUIRE(chunk2->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>() == parameter2_1);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == chunk2);

		// Insert CRC32C checksum.
		packet->WriteCRC32cChecksum();

		auto crc32cChecksum = packet->GetChecksum();

		// Packet length must be:
		// - Packet header: 12
		// - Chunk 1: 20
		// - Parameter 1.1: 8
		// - Parameter 1.2: 8
		// - Chunk 2: 4
		// - Parameter 2.1: 4 + 3 + 1 = 8
		// - Total: 60

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 60,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		/* Serialize the Packet. */

		packet->Serialize(sctpCommon::SerializeBuffer, packet->GetLength());

		std::memset(sctpCommon::FactoryBuffer, 0xAA, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ 60,
		  /*length*/ 60,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == chunk1);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == chunk2);

		/* Clone the Packet. */

		packet.reset(packet->Clone(sctpCommon::CloneBuffer, packet->GetLength()));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		const auto* obtainedChunk1 = reinterpret_cast<const RTC::SCTP::InitChunk*>(packet->GetChunkAt(0));

		// NOLINTNEXTLINE (readability-identifier-naming)
		const auto* obtainedParameter1_1 =
		  reinterpret_cast<const RTC::SCTP::IPv4AddressParameter*>(obtainedChunk1->GetParameterAt(0));

		// NOLINTNEXTLINE (readability-identifier-naming)
		const auto* obtainedParameter1_2 =
		  reinterpret_cast<const RTC::SCTP::CookiePreservativeParameter*>(
		    obtainedChunk1->GetParameterAt(1));

		const auto* obtainedChunk2 =
		  reinterpret_cast<const RTC::SCTP::HeartbeatRequestChunk*>(packet->GetChunkAt(1));

		// NOLINTNEXTLINE (readability-identifier-naming)
		const auto* obtainedParameter2_1 =
		  reinterpret_cast<const RTC::SCTP::HeartbeatInfoParameter*>(obtainedChunk2->GetParameterAt(0));

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ 60,
		  /*length*/ 60,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>() == obtainedChunk1);
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>() == obtainedChunk2);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ obtainedChunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20 + 8 + 8,
		  /*length*/ 20 + 8 + 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(obtainedChunk1->GetInitiateTag() == 87654321);
		REQUIRE(obtainedChunk1->GetAdvertisedReceiverWindowCredit() == 12345678);
		REQUIRE(obtainedChunk1->GetNumberOfOutboundStreams() == 11100);
		REQUIRE(obtainedChunk1->GetNumberOfInboundStreams() == 22200);
		REQUIRE(obtainedChunk1->GetInitialTsn() == 14141414);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ obtainedParameter1_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1_1->GetIPv4Address()[0] == 0xC0);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[1] == 0xA8);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[2] == 0x00);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[3] == 0x03);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ obtainedParameter1_2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1_2->GetLifeSpanIncrement() == 987654321);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ obtainedChunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4 + 8,
		  /*length*/ 4 + 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ obtainedParameter2_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter2_1->HasInfo() == true);
		REQUIRE(obtainedParameter2_1->GetInfoLength() == 3);
		REQUIRE(obtainedParameter2_1->GetInfo()[0] == 0x00);
		REQUIRE(obtainedParameter2_1->GetInfo()[1] == 0x01);
		REQUIRE(obtainedParameter2_1->GetInfo()[2] == 0x02);
	}

	SECTION("Factory() using AddChunk() succeeds")
	{
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, 1000) };

		packet->SetSourcePort(1);
		packet->SetDestinationPort(2);
		packet->SetVerificationTag(3);
		packet->SetChecksum(4);

		// 4 bytes Chunk.
		auto* chunk1 = RTC::SCTP::ShutdownCompleteChunk::Factory(sctpCommon::FactoryBuffer + 1000, 1000);

		chunk1->SetT(true);

		packet->AddChunk(chunk1);

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() != nullptr);
		// NOTE: The stored Chunk is not the same than the given one since it's
		// internally cloned.
		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() != chunk1);

		// Once added, we can delete the Chunk.
		delete chunk1;

		// Packet length must be:
		// - Packet header: 12
		// - Chunk 1: 4
		// - Total: 16

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 1000,
		  /*length*/ 16,
		  /*sourcePort*/ 1,
		  /*destinationPort*/ 2,
		  /*verificationTag*/ 3,
		  /*checksum*/ 4,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 1);

		const auto* obtainedChunk1 =
		  reinterpret_cast<const RTC::SCTP::ShutdownCompleteChunk*>(packet->GetChunkAt(0));

		REQUIRE(packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>() == obtainedChunk1);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ obtainedChunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(obtainedChunk1->GetT() == true);
	}

	SECTION("BuildChunkInPlace() throws if given Chunk exceeds Packet buffer length")
	{
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, 28) };

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 12,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		// Chunk 1: DATA, length: 16 bytes.
		auto* chunk1 = packet->BuildChunkInPlace<RTC::SCTP::DataChunk>();

		// Adding user data 10 bytes, must throw.
		REQUIRE_THROWS_AS(chunk1->SetUserDataPayload(sctpCommon::DataBuffer, 10), MediaSoupError);

		delete chunk1;

		// Chunk 2: INIT, length: 20 bytes. Must throw.
		REQUIRE_THROWS_AS(packet->BuildChunkInPlace<RTC::SCTP::InitChunk>(), MediaSoupError);

		CHECK_SCTP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 12,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);
	}

	SECTION("BuildChunkInPlace() and AddChunk() throw if the Packet needs consolidation")
	{
		std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Factory(
			sctpCommon::FactoryBuffer, 1000) };

		REQUIRE(packet->NeedsConsolidation() == false);

		const auto* chunk1 = packet->BuildChunkInPlace<RTC::SCTP::InitChunk>();

		REQUIRE(packet->NeedsConsolidation() == true);

		// We didn't call chunk1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(packet->BuildChunkInPlace<RTC::SCTP::ShutdownCompleteChunk>(), MediaSoupError);

		const auto* chunk2 = RTC::SCTP::ShutdownCompleteChunk::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// We didn't call chunk1->Consolidate() yet so this must throw.
		REQUIRE_THROWS_AS(packet->AddChunk(chunk2), MediaSoupError);

		delete chunk2;

		chunk1->Consolidate();

		REQUIRE(packet->NeedsConsolidation() == false);

		// This shouldn't throw now.
		const auto* chunk3 = packet->BuildChunkInPlace<RTC::SCTP::ShutdownCompleteChunk>();

		REQUIRE(packet->NeedsConsolidation() == true);

		chunk3->Consolidate();

		REQUIRE(packet->NeedsConsolidation() == false);

		const auto* chunk4 = RTC::SCTP::ShutdownCompleteChunk::Factory(
		  sctpCommon::FactoryBuffer + 1000, sizeof(sctpCommon::FactoryBuffer));

		// This shouldn't throw now.
		packet->AddChunk(chunk4);

		REQUIRE(packet->NeedsConsolidation() == false);

		delete chunk4;
	}
}
