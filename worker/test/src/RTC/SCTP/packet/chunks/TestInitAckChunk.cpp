#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

// NOTE: Simplified since it's similar to InitChunk.
SCENARIO("SCTP Init Acknowledgement (2)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("InitAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:2 (INIT_ACK), Flags: 0b00000000, Length: 28
			0x02, 0b00000000, 0x00, 0x1C,
			// Initiate Tag: 287454020,
			0x11, 0x22, 0x33, 0x44,
			// Advertised Receiver Window Credit: 4278216311
			0xFF, 0x00, 0x66, 0x77,
			// Number of Outbound Streams: 4660, Number of Inbound Streams: 22136
			0x12, 0x34, 0x56, 0x78,
			// Initial TSN: 2882339074
			0xAB, 0xCD, 0x01, 0x02,
			// Parameter 1: Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address: "2.3.4.5"
			0x02, 0x03, 0x04, 0x05,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = InitAckChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(chunk->GetInitialTsn() == 2882339074);

		auto* parameter1 = reinterpret_cast<const IPv4AddressParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->GetIPv4Address()[0] == 0x02);
		REQUIRE(parameter1->GetIPv4Address()[1] == 0x03);
		REQUIRE(parameter1->GetIPv4Address()[2] == 0x04);
		REQUIRE(parameter1->GetIPv4Address()[3] == 0x05);

		delete chunk;
	}

	SECTION("InitAckChunk::Factory() succeeds")
	{
		auto* chunk = InitAckChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 0);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 0);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 0);
		REQUIRE(chunk->GetInitialTsn() == 0);

		/* Modify it and add Parameters. */

		chunk->SetInitiateTag(1111111110);
		chunk->SetAdvertisedReceiverWindowCredit(2222222220);
		chunk->SetNumberOfOutboundStreams(1234);
		chunk->SetNumberOfInboundStreams(5678);
		chunk->SetInitialTsn(3333333330);

		auto* parameter1 = chunk->BuildParameterInPlace<IPv4AddressParameter>();

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer1[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter1->SetIPv4Address(ipBuffer1);
		parameter1->Consolidate();

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 28,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 1111111110);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 1234);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 5678);
		REQUIRE(chunk->GetInitialTsn() == 3333333330);

		const auto* addedParameter1 =
		  reinterpret_cast<const IPv4AddressParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ addedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter1->GetIPv4Address()[0] == 0x0B);
		REQUIRE(addedParameter1->GetIPv4Address()[1] == 0x16);
		REQUIRE(addedParameter1->GetIPv4Address()[2] == 0x21);
		REQUIRE(addedParameter1->GetIPv4Address()[3] == 0x2C);

		delete chunk;
	}
}
