#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
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
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP Packet", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("Packet::Parse() without Chunks succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Source Port: 10000, Destination Port: 15999
			0x27, 0x10, 0x3E, 0x7F,
			// Verification Tag: 4294967285
			0xFF, 0xFF, 0xFF, 0xF5,
			// Checksum: 5
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format on

		auto* packet = Packet::Parse(buffer, sizeof(buffer));

		// NOTE: Obviously the Checksum CRC32C validation fails since Checksum is
		// totally random.
		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<DataChunk>() == nullptr);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(packet->BuildChunkInPlace<DataChunk>(), MediaSoupError);
		REQUIRE_THROWS_AS(packet->SetSourcePort(10), MediaSoupError);
		REQUIRE_THROWS_AS(packet->SetDestinationPort(9999), MediaSoupError);
		REQUIRE_THROWS_AS(packet->SetVerificationTag(12345), MediaSoupError);
		REQUIRE_THROWS_AS(packet->SetChecksum(6666), MediaSoupError);
		REQUIRE_THROWS_AS(packet->SetCRC32cChecksum(), MediaSoupError);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<DataChunk>() == nullptr);

		/* Insert CRC32C checksum. */

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		/* Clone it. */

		auto* clonedPacket = packet->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete packet;

		CHECK_PACKET(
		  /*packet*/ clonedPacket,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(clonedPacket->GetFirstChunkOfType<DataChunk>() == nullptr);

		delete clonedPacket;
	}

	SECTION("Packet::Parse() with Chunks succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
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

		auto* packet = Packet::Parse(buffer, sizeof(buffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 52,
		  /*frozen*/ true,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(packet->GetFirstChunkOfType<DataChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<UnknownChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<HeartbeatAckChunk>() != nullptr);
		REQUIRE(packet->GetFirstChunkOfType<InitChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<HeartbeatRequestChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<ShutdownCompleteChunk>() == nullptr);

		auto* chunk1 = reinterpret_cast<const DataChunk*>(packet->GetChunkAt(0));

		REQUIRE(packet->GetFirstChunkOfType<DataChunk>() == chunk1);

		CHECK_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk1->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk1->HasUserData() == true);
		REQUIRE(chunk1->GetUserDataLength() == 2);
		REQUIRE(chunk1->GetUserData()[0] == 0xAB);
		REQUIRE(chunk1->GetUserData()[1] == 0xCD);

		auto* chunk2 = reinterpret_cast<const UnknownChunk*>(packet->GetChunkAt(1));

		REQUIRE(packet->GetFirstChunkOfType<UnknownChunk>() == chunk2);

		CHECK_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
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

		auto* chunk3 = reinterpret_cast<const HeartbeatAckChunk*>(packet->GetChunkAt(2));

		REQUIRE(packet->GetFirstChunkOfType<HeartbeatAckChunk>() == chunk3);

		CHECK_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* parameter3_1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(packet->BuildChunkInPlace<DataChunk>(), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetI(false), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetU(false), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetB(false), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetE(false), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetTsn(1234), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetStreamIdentifierS(1234), MediaSoupError);
		REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetStreamSequenceNumberN(1234), MediaSoupError);
		REQUIRE_THROWS_AS(
		  const_cast<DataChunk*>(chunk1)->SetPayloadProtocolIdentifier(1234), MediaSoupError);

		/* Serialize it. */

		packet->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 52,
		  /*frozen*/ false,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(packet->GetFirstChunkOfType<DataChunk>() == chunk1);
		REQUIRE(packet->GetFirstChunkOfType<UnknownChunk>() == chunk2);
		REQUIRE(packet->GetFirstChunkOfType<HeartbeatAckChunk>() == chunk3);
		REQUIRE(packet->GetFirstChunkOfType<InitChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<HeartbeatRequestChunk>() == nullptr);
		REQUIRE(packet->GetFirstChunkOfType<ShutdownCompleteChunk>() == nullptr);

		chunk1 = reinterpret_cast<const DataChunk*>(packet->GetChunkAt(0));

		CHECK_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk1->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk1->HasUserData() == true);
		REQUIRE(chunk1->GetUserDataLength() == 2);
		REQUIRE(chunk1->GetUserData()[0] == 0xAB);
		REQUIRE(chunk1->GetUserData()[1] == 0xCD);

		chunk2 = reinterpret_cast<const UnknownChunk*>(packet->GetChunkAt(1));

		CHECK_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
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

		chunk3 = reinterpret_cast<const HeartbeatAckChunk*>(packet->GetChunkAt(2));

		CHECK_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter3_1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);

		/* Clone it. */

		auto* clonedPacket = packet->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete packet;

		CHECK_PACKET(
		  /*packet*/ clonedPacket,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 52,
		  /*frozen*/ false,
		  /*sourcePort*/ 10000,
		  /*destinationPort*/ 15999,
		  /*verificationTag*/ 4294967285,
		  /*checksum*/ 5,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 3);

		REQUIRE(clonedPacket->GetFirstChunkOfType<DataChunk>() != nullptr);
		REQUIRE(clonedPacket->GetFirstChunkOfType<UnknownChunk>() != nullptr);
		REQUIRE(clonedPacket->GetFirstChunkOfType<HeartbeatAckChunk>() != nullptr);
		REQUIRE(clonedPacket->GetFirstChunkOfType<InitChunk>() == nullptr);
		REQUIRE(clonedPacket->GetFirstChunkOfType<HeartbeatRequestChunk>() == nullptr);
		REQUIRE(clonedPacket->GetFirstChunkOfType<ShutdownCompleteChunk>() == nullptr);

		chunk1 = reinterpret_cast<const DataChunk*>(clonedPacket->GetChunkAt(0));

		REQUIRE(clonedPacket->GetFirstChunkOfType<DataChunk>() == chunk1);

		CHECK_CHUNK(
		  /*chunk*/ chunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk1->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk1->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk1->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk1->HasUserData() == true);
		REQUIRE(chunk1->GetUserDataLength() == 2);
		REQUIRE(chunk1->GetUserData()[0] == 0xAB);
		REQUIRE(chunk1->GetUserData()[1] == 0xCD);

		chunk2 = reinterpret_cast<const UnknownChunk*>(clonedPacket->GetChunkAt(1));

		REQUIRE(clonedPacket->GetFirstChunkOfType<UnknownChunk>() == chunk2);

		CHECK_CHUNK(
		  /*chunk*/ chunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
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

		chunk3 = reinterpret_cast<const HeartbeatAckChunk*>(clonedPacket->GetChunkAt(2));

		REQUIRE(clonedPacket->GetFirstChunkOfType<HeartbeatAckChunk>() == chunk3);

		CHECK_CHUNK(
		  /*chunk*/ chunk3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter3_1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk3->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3_1->HasInfo() == true);
		REQUIRE(parameter3_1->GetInfoLength() == 2);
		REQUIRE(parameter3_1->GetInfo()[0] == 0x11);
		REQUIRE(parameter3_1->GetInfo()[1] == 0x22);
		// These should be padding.
		REQUIRE(parameter3_1->GetInfo()[2] == 0x00);
		REQUIRE(parameter3_1->GetInfo()[3] == 0x00);

		delete clonedPacket;
	}

	SECTION("Packet::Factory() with Chunks succeeds")
	{
		auto* packet = Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		REQUIRE(packet->GetFirstChunkOfType<InitChunk>() == nullptr);

		/* Modify the Packet and add Chunks. */

		packet->SetSourcePort(1000);
		packet->SetDestinationPort(6000);
		packet->SetVerificationTag(12345678);
		packet->SetChecksum(0);

		// Chunk 1: INIT, length: 20 bytes.
		auto* chunk1 = packet->BuildChunkInPlace<InitChunk>();

		chunk1->SetInitiateTag(87654321);
		chunk1->SetAdvertisedReceiverWindowCredit(12345678);
		chunk1->SetNumberOfOutboundStreams(11100);
		chunk1->SetNumberOfInboundStreams(22200);
		chunk1->SetInitialTsn(14141414);

		// Parameter 1.1: IPV4_ADDRESS, length: 8 bytes.
		auto* parameter1_1 = chunk1->BuildParameterInPlace<IPv4AddressParameter>();

		// 192.168.0.3 IPv4 in network order.
		uint8_t ipBuffer[] = { 0xC0, 0xA8, 0x00, 0x03 };

		parameter1_1->SetIPv4Address(ipBuffer);
		parameter1_1->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<IPv4AddressParameter>() == parameter1_1);

		// Parameter 1.2: COOKIE_PRESERVATIVE, length: 8 bytes.
		auto* parameter1_2 = chunk1->BuildParameterInPlace<CookiePreservativeParameter>();

		parameter1_2->SetLifeSpanIncrement(987654321);
		parameter1_2->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<CookiePreservativeParameter>() == parameter1_2);

		// Consolidate Chunk 1 after consolidating its Parameters 1.1 and 1.2.
		chunk1->Consolidate();

		REQUIRE(chunk1->GetFirstParameterOfType<IPv4AddressParameter>() == parameter1_1);
		REQUIRE(chunk1->GetFirstParameterOfType<CookiePreservativeParameter>() == parameter1_2);

		REQUIRE(packet->GetFirstChunkOfType<InitChunk>() == chunk1);

		// Chunk 2: HEARTBEAT_REQUEST, length: 4 bytes.
		auto* chunk2 = packet->BuildChunkInPlace<HeartbeatRequestChunk>();

		// Parameter 2.1: HEARTBEAT_INFO, length: 4 bytes.
		auto* parameter2_1 = chunk2->BuildParameterInPlace<HeartbeatInfoParameter>();

		// Parameter 2.1: Add 3 bytes of info + 1 byte of padding.
		parameter2_1->SetInfo(DataBuffer, 3);
		parameter2_1->Consolidate();

		REQUIRE(chunk2->GetFirstParameterOfType<HeartbeatInfoParameter>() == parameter2_1);

		std::memset(DataBuffer, 0xFF, 3);

		// Consolidate the Chunk after consolidating its Parameters.
		chunk2->Consolidate();

		REQUIRE(chunk2->GetFirstParameterOfType<HeartbeatInfoParameter>() == parameter2_1);

		REQUIRE(packet->GetFirstChunkOfType<HeartbeatRequestChunk>() == chunk2);

		// Insert CRC32C checksum.
		packet->SetCRC32cChecksum();

		auto crc32cChecksum = packet->GetChecksum();

		// Packet length must be:
		// - Packet header: 12
		// - Chunk 1: 20
		// - Parameter 1.1: 8
		// - Parameter 1.2: 8
		// - Chunk 2: 4
		// - Parameter 2.1: 4 + 3 + 1 = 8
		// - Total: 60

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 60,
		  /*frozen*/ false,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		/* Serialize the Packet. */

		packet->Serialize(SerializeBuffer, packet->GetLength());

		std::memset(FactoryBuffer, 0xAA, sizeof(FactoryBuffer));

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ 60,
		  /*length*/ 60,
		  /*frozen*/ false,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		REQUIRE(packet->GetFirstChunkOfType<InitChunk>() == chunk1);
		REQUIRE(packet->GetFirstChunkOfType<HeartbeatRequestChunk>() == chunk2);

		/* Clone the Packet. */

		auto* clonedPacket = packet->Clone(CloneBuffer, packet->GetLength());

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete packet;

		auto* obtainedChunk1 = reinterpret_cast<const InitChunk*>(clonedPacket->GetChunkAt(0));

		auto* obtainedParameter1_1 =
		  reinterpret_cast<const IPv4AddressParameter*>(obtainedChunk1->GetParameterAt(0));

		auto* obtainedParameter1_2 =
		  reinterpret_cast<const CookiePreservativeParameter*>(obtainedChunk1->GetParameterAt(1));

		auto* obtainedChunk2 =
		  reinterpret_cast<const HeartbeatRequestChunk*>(clonedPacket->GetChunkAt(1));

		auto* obtainedParameter2_1 =
		  reinterpret_cast<const HeartbeatInfoParameter*>(obtainedChunk2->GetParameterAt(0));

		CHECK_PACKET(
		  /*packet*/ clonedPacket,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ 60,
		  /*length*/ 60,
		  /*frozen*/ false,
		  /*sourcePort*/ 1000,
		  /*destinationPort*/ 6000,
		  /*verificationTag*/ 12345678,
		  /*checksum*/ crc32cChecksum,
		  /*hasValidCrc32cChecksum*/ true,
		  /*chunksCount*/ 2);

		REQUIRE(clonedPacket->GetFirstChunkOfType<InitChunk>() == obtainedChunk1);
		REQUIRE(clonedPacket->GetFirstChunkOfType<HeartbeatRequestChunk>() == obtainedChunk2);

		CHECK_CHUNK(
		  /*chunk*/ obtainedChunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20 + 8 + 8,
		  /*length*/ 20 + 8 + 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
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

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter1_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1_1->GetIPv4Address()[0] == 0xC0);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[1] == 0xA8);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[2] == 0x00);
		REQUIRE(obtainedParameter1_1->GetIPv4Address()[3] == 0x03);

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter1_2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1_2->GetLifeSpanIncrement() == 987654321);

		CHECK_CHUNK(
		  /*chunk*/ obtainedChunk2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4 + 8,
		  /*length*/ 4 + 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter2_1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter2_1->HasInfo() == true);
		REQUIRE(obtainedParameter2_1->GetInfoLength() == 3);
		REQUIRE(obtainedParameter2_1->GetInfo()[0] == 0x00);
		REQUIRE(obtainedParameter2_1->GetInfo()[1] == 0x01);
		REQUIRE(obtainedParameter2_1->GetInfo()[2] == 0x02);

		delete clonedPacket;
	}

	SECTION("Packet::Factory() using AddChunk() succeeds")
	{
		auto* packet = Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		packet->SetSourcePort(1);
		packet->SetDestinationPort(2);
		packet->SetVerificationTag(3);
		packet->SetChecksum(4);

		// 4 bytes Chunk.
		auto* chunk1 = ShutdownCompleteChunk::Factory(FactoryBuffer + 1000, sizeof(FactoryBuffer));

		chunk1->SetT(true);

		packet->AddChunk(chunk1);

		REQUIRE(packet->GetFirstChunkOfType<ShutdownCompleteChunk>() != nullptr);
		// NOTE: The stored Chunk is not the same than the given one since it's
		// internally cloned.
		REQUIRE(packet->GetFirstChunkOfType<ShutdownCompleteChunk>() != chunk1);

		// Once added, we can delete the Chunk.
		delete chunk1;

		// Packet length must be:
		// - Packet header: 12
		// - Chunk 1: 4
		// - Total: 16

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*sourcePort*/ 1,
		  /*destinationPort*/ 2,
		  /*verificationTag*/ 3,
		  /*checksum*/ 4,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 1);

		auto* obtainedChunk1 = reinterpret_cast<const ShutdownCompleteChunk*>(packet->GetChunkAt(0));

		REQUIRE(packet->GetFirstChunkOfType<ShutdownCompleteChunk>() == obtainedChunk1);

		CHECK_CHUNK(
		  /*chunk*/ obtainedChunk1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(obtainedChunk1->GetT() == true);

		delete packet;
	}

	SECTION("Packet::BuildChunkInPlace() throws if given Chunk exceeds Packet buffer length")
	{
		auto* packet = Packet::Factory(FactoryBuffer, 28);

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		// Chunk 1: DATA, length: 16 bytes.
		auto* chunk1 = packet->BuildChunkInPlace<DataChunk>();

		// Adding user data 10 bytes, must throw.
		REQUIRE_THROWS_AS(chunk1->SetUserData(DataBuffer, 10), MediaSoupError);

		delete chunk1;

		// Chunk 2: INIT, length: 20 bytes. Must throw.
		REQUIRE_THROWS_AS(packet->BuildChunkInPlace<InitChunk>(), MediaSoupError);

		CHECK_PACKET(
		  /*packet*/ packet,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 28,
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*sourcePort*/ 0,
		  /*destinationPort*/ 0,
		  /*verificationTag*/ 0,
		  /*checksum*/ 0,
		  /*hasValidCrc32cChecksum*/ false,
		  /*chunksCount*/ 0);

		delete packet;
	}
}
