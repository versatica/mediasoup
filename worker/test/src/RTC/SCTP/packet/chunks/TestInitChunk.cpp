#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedAddressTypesParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Init Chunk (1)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("InitChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:1 (INIT), Flags: 0b00000000, Length: 56
			0x01, 0b00000000, 0x00, 0x38,
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
			// Type:6 (IPV6_ADDRESS), Length: 20
			0x00, 0x06, 0x00, 0x14,
			// Parameter 2: IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73, 0x34,
			// Parameter 3: Type:9 (COOKIE_PRESERVATIVE), Length: 8
			0x00, 0x09, 0x00, 0x08,
			// Suggested Cookie Life-Span Increment: 556942164
			0x21, 0x32, 0x43, 0x54,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		auto* chunk = InitChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 56,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 3,
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

		auto* parameter2 = reinterpret_cast<const IPv6AddressParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		auto* parameter3 = reinterpret_cast<const CookiePreservativeParameter*>(chunk->GetParameterAt(2));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->BuildParameterInPlace<IPv4AddressParameter>(), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetInitiateTag(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetAdvertisedReceiverWindowCredit(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetNumberOfOutboundStreams(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetNumberOfInboundStreams(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetInitialTsn(1234), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 3,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(chunk->GetInitialTsn() == 2882339074);

		parameter1 = reinterpret_cast<const IPv4AddressParameter*>(chunk->GetParameterAt(0));

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

		parameter2 = reinterpret_cast<const IPv6AddressParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		parameter3 = reinterpret_cast<const CookiePreservativeParameter*>(chunk->GetParameterAt(2));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 3,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetInitiateTag() == 287454020);
		REQUIRE(clonedChunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(clonedChunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(clonedChunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(clonedChunk->GetInitialTsn() == 2882339074);

		parameter1 = reinterpret_cast<const IPv4AddressParameter*>(clonedChunk->GetParameterAt(0));

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

		parameter2 = reinterpret_cast<const IPv6AddressParameter*>(clonedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		parameter3 = reinterpret_cast<const CookiePreservativeParameter*>(clonedChunk->GetParameterAt(2));

		CHECK_PARAMETER(
		  /*parameter*/ parameter3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		delete clonedChunk;
	}

	SECTION("InitChunk::Factory() succeeds")
	{
		auto* chunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
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

		auto* parameter2 = chunk->BuildParameterInPlace<IPv6AddressParameter>();

		// 2345:0425:2CA1:0000:0000:0567:5673:23b5 IPv6 in network order.
		uint8_t ipBuffer2[] = { 0x23, 0x45, 0x04, 0x25, 0x2C, 0xA1, 0x00, 0x00,
			                      0x00, 0x00, 0x05, 0x67, 0x56, 0x73, 0x23, 0xB5 };

		parameter2->SetIPv6Address(ipBuffer2);
		parameter2->Consolidate();

		auto* parameter3 = chunk->BuildParameterInPlace<CookiePreservativeParameter>();

		parameter3->SetLifeSpanIncrement(876543210);
		parameter3->Consolidate();

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 3,
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

		const auto* addedParameter2 =
		  reinterpret_cast<const IPv6AddressParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ addedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter2->GetIPv6Address()[0] == 0x23);
		REQUIRE(addedParameter2->GetIPv6Address()[1] == 0x45);
		REQUIRE(addedParameter2->GetIPv6Address()[2] == 0x04);
		REQUIRE(addedParameter2->GetIPv6Address()[3] == 0x25);
		REQUIRE(addedParameter2->GetIPv6Address()[15] == 0xB5);

		const auto* addedParameter3 =
		  reinterpret_cast<const CookiePreservativeParameter*>(chunk->GetParameterAt(2));

		CHECK_PARAMETER(
		  /*parameter*/ addedParameter3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter3->GetLifeSpanIncrement() == 876543210);

		/* Parse itself and compare. */

		auto* parsedChunk = InitChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 56,
		  /*length*/ 56,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 3,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetInitiateTag() == 1111111110);
		REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
		REQUIRE(parsedChunk->GetNumberOfOutboundStreams() == 1234);
		REQUIRE(parsedChunk->GetNumberOfInboundStreams() == 5678);
		REQUIRE(parsedChunk->GetInitialTsn() == 3333333330);

		const auto* parsedParameter1 =
		  reinterpret_cast<const IPv4AddressParameter*>(parsedChunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter1->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parsedParameter1->GetIPv4Address()[1] == 0x16);
		REQUIRE(parsedParameter1->GetIPv4Address()[2] == 0x21);
		REQUIRE(parsedParameter1->GetIPv4Address()[3] == 0x2C);

		const auto* parsedParameter2 =
		  reinterpret_cast<const IPv6AddressParameter*>(parsedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter2->GetIPv6Address()[0] == 0x23);
		REQUIRE(parsedParameter2->GetIPv6Address()[1] == 0x45);
		REQUIRE(parsedParameter2->GetIPv6Address()[2] == 0x04);
		REQUIRE(parsedParameter2->GetIPv6Address()[3] == 0x25);
		REQUIRE(parsedParameter2->GetIPv6Address()[15] == 0xB5);

		const auto* parsedParameter3 =
		  reinterpret_cast<const CookiePreservativeParameter*>(parsedChunk->GetParameterAt(2));

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter3,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter3->GetLifeSpanIncrement() == 876543210);

		delete parsedChunk;
	}

	SECTION("InitChunk::Factory() using AddParameter() succeeds")
	{
		auto* chunk = InitChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		chunk->SetInitiateTag(1);
		chunk->SetAdvertisedReceiverWindowCredit(2);
		chunk->SetNumberOfOutboundStreams(3);
		chunk->SetNumberOfInboundStreams(4);
		chunk->SetInitialTsn(5);

		auto* parameter1 =
		  CookiePreservativeParameter::Factory(FactoryBuffer + 1000, sizeof(FactoryBuffer));

		// 8 bytes Parameter.
		parameter1->SetLifeSpanIncrement(123456);

		chunk->AddParameter(parameter1);

		// Once added, we can delete the Parameter.
		delete parameter1;

		// Chunk length must be:
		// - Chunk header: 20
		// - Parameter 1: 8
		// - Total: 28

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 28,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 1,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* obtainedParameter1 =
		  reinterpret_cast<const CookiePreservativeParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1->GetLifeSpanIncrement() == 123456);

		// 4 bytes Parameter.
		auto* parameter2 =
		  SupportedAddressTypesParameter::Factory(FactoryBuffer + 1000, sizeof(FactoryBuffer));

		// Add 6 bytes.
		parameter2->AddAddressType(1111);
		parameter2->AddAddressType(2222);
		parameter2->AddAddressType(3333);

		chunk->AddParameter(parameter2);

		// Once added, we can delete the Parameter.
		delete parameter2;

		// Chunk length must be:
		// - Chunk header: 20
		// - Parameter 1: 8
		// - Parameter 2: 12
		// - Total: 40

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 40,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* obtainedParameter2 =
		  reinterpret_cast<const SupportedAddressTypesParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter2->GetNumberOfAddressTypes() == 3);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(0) == 1111);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(1) == 2222);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(2) == 3333);

		/* Parse itself and compare. */

		auto* parsedChunk = InitChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 40,
		  /*length*/ 40,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetInitiateTag() == 1);
		REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 2);
		REQUIRE(parsedChunk->GetNumberOfOutboundStreams() == 3);
		REQUIRE(parsedChunk->GetNumberOfInboundStreams() == 4);
		REQUIRE(parsedChunk->GetInitialTsn() == 5);

		obtainedParameter1 =
		  reinterpret_cast<const CookiePreservativeParameter*>(parsedChunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter1->GetLifeSpanIncrement() == 123456);

		obtainedParameter2 =
		  reinterpret_cast<const SupportedAddressTypesParameter*>(parsedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ obtainedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(obtainedParameter2->GetNumberOfAddressTypes() == 3);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(0) == 1111);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(1) == 2222);
		REQUIRE(obtainedParameter2->GetAddressTypeAt(2) == 3333);

		delete parsedChunk;
	}
}
