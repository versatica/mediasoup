#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/CookiePreservativeChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv4AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv6AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/UnknownChunkParameter.hpp"
#include "RTC/SCTP/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/chunks/DataChunk.hpp"
#include "RTC/SCTP/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/chunks/HeartbeatChunk.hpp"
#include "RTC/SCTP/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/chunks/InitChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/chunks/UnknownChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

thread_local static uint8_t ChunkFactoryBuffer[66661];
thread_local static uint8_t ChunkSerializeBuffer[66662];
thread_local static uint8_t ChunkCloneBuffer[66663];
thread_local static uint8_t ChunkCustomDataBuffer[66664];
thread_local static uint8_t ThrowBuffer[66665];

static void resetBuffers();

static void checkChunk(
  const Chunk* chunk,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  Chunk::ActionForUnknownChunkType actionForUnknownChunkType,
  uint8_t flags,
  size_t parametersCount);

static void checkChunkParameter(
  const ChunkParameter* parameter,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength);

SCENARIO("SCTP Payload Data Chunk (0)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("DataChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:0 (DATA), I:1, U:0, B:1, E:1, Length: 19
			0x00, 0b00001011, 0x00, 0x13,
			// TSN: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Stream Identifier S: 0xFF00, Stream Sequence Number n: 0x6677
			0xFF, 0x00, 0x66, 0x77,
			// Payload Protocol Identifier: 0x12341234
			0x12, 0x34, 0x12, 0x34,
			// User Data (2 bytes): 0xABCD, 1 byte of padding
			0xAB, 0xCD, 0xEF, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = DataChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetTSN() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetI(true), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetE(true), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetTSN(12345678), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetStreamIdentifierS(9988), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetStreamSequenceNumberN(2211), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetPayloadProtocolIdentifier(987654321), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetUserData(ChunkCustomDataBuffer, 3), MediaSoupError);
		REQUIRE_THROWS_AS(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::IPV4_ADDRESS), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetTSN() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*parametersCount*/ 0);

		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetU() == false);
		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetTSN() == 0x11223344);
		REQUIRE(clonedChunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(clonedChunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(clonedChunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(clonedChunk->HasUserData() == true);
		REQUIRE(clonedChunk->GetUserDataLength() == 3);
		REQUIRE(clonedChunk->GetUserData()[0] == 0xAB);
		REQUIRE(clonedChunk->GetUserData()[1] == 0xCD);
		REQUIRE(clonedChunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(clonedChunk->GetUserData()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("DataChunk::Factory() succeeds")
	{
		auto* chunk = DataChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetTSN() == 0);
		REQUIRE(chunk->GetStreamIdentifierS() == 0);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTSN(12345678);
		chunk->SetStreamIdentifierS(9988);
		chunk->SetStreamSequenceNumberN(2211);
		chunk->SetPayloadProtocolIdentifier(987654321);
		// 3 bytes + 1 byte of padding.
		chunk->SetUserData(ChunkCustomDataBuffer, 3);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == false);
		REQUIRE(chunk->GetE() == true);
		REQUIRE(chunk->GetTSN() == 12345678);
		REQUIRE(chunk->GetStreamIdentifierS() == 9988);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 2211);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 987654321);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0x00);
		REQUIRE(chunk->GetUserData()[1] == 0x01);
		REQUIRE(chunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = DataChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 16 + 3 + 1,
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001001,
		  /*parametersCount*/ 0);

		REQUIRE(parsedChunk->GetI() == true);
		REQUIRE(parsedChunk->GetU() == false);
		REQUIRE(parsedChunk->GetB() == false);
		REQUIRE(parsedChunk->GetE() == true);
		REQUIRE(parsedChunk->GetTSN() == 12345678);
		REQUIRE(parsedChunk->GetStreamIdentifierS() == 9988);
		REQUIRE(parsedChunk->GetStreamSequenceNumberN() == 2211);
		REQUIRE(parsedChunk->GetPayloadProtocolIdentifier() == 987654321);
		REQUIRE(parsedChunk->HasUserData() == true);
		REQUIRE(parsedChunk->GetUserDataLength() == 3);
		REQUIRE(parsedChunk->GetUserData()[0] == 0x00);
		REQUIRE(parsedChunk->GetUserData()[1] == 0x01);
		REQUIRE(parsedChunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(parsedChunk->GetUserData()[3] == 0x00);

		delete parsedChunk;
	}

	SECTION("DataChunk::SetUserData() throws if userDataLength is too big")
	{
		auto* chunk = DataChunk::Factory(ThrowBuffer, sizeof(ThrowBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE_THROWS_AS(chunk->SetUserData(ThrowBuffer, 65535), MediaSoupError);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		delete chunk;
	}
}

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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 56,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 3);

		REQUIRE(chunk->GetInitiateTag() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(chunk->GetInitialTSN() == 2882339074);

		auto* parameter1 = reinterpret_cast<const IPv4AddressChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter1->GetIPv4Address()[0] == 0x02);
		REQUIRE(parameter1->GetIPv4Address()[1] == 0x03);
		REQUIRE(parameter1->GetIPv4Address()[2] == 0x04);
		REQUIRE(parameter1->GetIPv4Address()[3] == 0x05);

		auto* parameter2 = reinterpret_cast<const IPv6AddressChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		auto* parameter3 =
		  reinterpret_cast<const CookiePreservativeChunkParameter*>(chunk->GetParameterAt(2));

		checkChunkParameter(
		  /*parameter*/ parameter3,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::IPV4_ADDRESS), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetInitiateTag(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetAdvertisedReceiverWindowCredit(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetNumberOfOutboundStreams(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetNumberOfInboundStreams(1234), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetInitialTSN(1234), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 3);

		REQUIRE(chunk->GetInitiateTag() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(chunk->GetInitialTSN() == 2882339074);

		parameter1 = reinterpret_cast<const IPv4AddressChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter1->GetIPv4Address()[0] == 0x02);
		REQUIRE(parameter1->GetIPv4Address()[1] == 0x03);
		REQUIRE(parameter1->GetIPv4Address()[2] == 0x04);
		REQUIRE(parameter1->GetIPv4Address()[3] == 0x05);

		parameter2 = reinterpret_cast<const IPv6AddressChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		parameter3 = reinterpret_cast<const CookiePreservativeChunkParameter*>(chunk->GetParameterAt(2));

		checkChunkParameter(
		  /*parameter*/ parameter3,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 3);

		REQUIRE(clonedChunk->GetInitiateTag() == 287454020);
		REQUIRE(clonedChunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(clonedChunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(clonedChunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(clonedChunk->GetInitialTSN() == 2882339074);

		parameter1 = reinterpret_cast<const IPv4AddressChunkParameter*>(clonedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter1->GetIPv4Address()[0] == 0x02);
		REQUIRE(parameter1->GetIPv4Address()[1] == 0x03);
		REQUIRE(parameter1->GetIPv4Address()[2] == 0x04);
		REQUIRE(parameter1->GetIPv4Address()[3] == 0x05);

		parameter2 = reinterpret_cast<const IPv6AddressChunkParameter*>(clonedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter2->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter2->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter2->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter2->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter2->GetIPv6Address()[15] == 0x34);

		parameter3 =
		  reinterpret_cast<const CookiePreservativeChunkParameter*>(clonedChunk->GetParameterAt(2));

		checkChunkParameter(
		  /*parameter*/ parameter3,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter3->GetLifeSpanIncrement() == 556942164);

		delete clonedChunk;
	}

	SECTION("InitChunk::Factory() succeeds")
	{
		auto* chunk = InitChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 0);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 0);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 0);
		REQUIRE(chunk->GetInitialTSN() == 0);

		/* Modify it and add Parameters. */

		chunk->SetInitiateTag(1111111110);
		chunk->SetAdvertisedReceiverWindowCredit(2222222220);
		chunk->SetNumberOfOutboundStreams(1234);
		chunk->SetNumberOfInboundStreams(5678);
		chunk->SetInitialTSN(3333333330);

		auto* parameter1 = reinterpret_cast<IPv4AddressChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::IPV4_ADDRESS));

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer1[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter1->SetIPv4Address(ipBuffer1);
		parameter1->Consolidate();

		auto* parameter2 = reinterpret_cast<IPv6AddressChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::IPV6_ADDRESS));

		// 2345:0425:2CA1:0000:0000:0567:5673:23b5 IPv6 in network order.
		uint8_t ipBuffer2[] = { 0x23, 0x45, 0x04, 0x25, 0x2C, 0xA1, 0x00, 0x00,
			                      0x00, 0x00, 0x05, 0x67, 0x56, 0x73, 0x23, 0xB5 };

		parameter2->SetIPv6Address(ipBuffer2);
		parameter2->Consolidate();

		auto* parameter3 = reinterpret_cast<CookiePreservativeChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE));

		parameter3->SetLifeSpanIncrement(876543210);
		parameter3->Consolidate();

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 56,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 3);

		REQUIRE(chunk->GetInitiateTag() == 1111111110);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 1234);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 5678);
		REQUIRE(chunk->GetInitialTSN() == 3333333330);

		const auto* addedParameter1 =
		  reinterpret_cast<const IPv4AddressChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ addedParameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(addedParameter1->GetIPv4Address()[0] == 0x0B);
		REQUIRE(addedParameter1->GetIPv4Address()[1] == 0x16);
		REQUIRE(addedParameter1->GetIPv4Address()[2] == 0x21);
		REQUIRE(addedParameter1->GetIPv4Address()[3] == 0x2C);

		const auto* addedParameter2 =
		  reinterpret_cast<const IPv6AddressChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ addedParameter2,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(addedParameter2->GetIPv6Address()[0] == 0x23);
		REQUIRE(addedParameter2->GetIPv6Address()[1] == 0x45);
		REQUIRE(addedParameter2->GetIPv6Address()[2] == 0x04);
		REQUIRE(addedParameter2->GetIPv6Address()[3] == 0x25);
		REQUIRE(addedParameter2->GetIPv6Address()[15] == 0xB5);

		const auto* addedParameter3 =
		  reinterpret_cast<const CookiePreservativeChunkParameter*>(chunk->GetParameterAt(2));

		checkChunkParameter(
		  /*parameter*/ addedParameter3,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(addedParameter3->GetLifeSpanIncrement() == 876543210);

		/* Parse itself and compare. */

		auto* parsedChunk = InitChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 56,
		  /*length*/ 56,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 3);

		REQUIRE(parsedChunk->GetInitiateTag() == 1111111110);
		REQUIRE(parsedChunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
		REQUIRE(parsedChunk->GetNumberOfOutboundStreams() == 1234);
		REQUIRE(parsedChunk->GetNumberOfInboundStreams() == 5678);
		REQUIRE(parsedChunk->GetInitialTSN() == 3333333330);

		const auto* parsedParameter1 =
		  reinterpret_cast<const IPv4AddressChunkParameter*>(parsedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parsedParameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parsedParameter1->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parsedParameter1->GetIPv4Address()[1] == 0x16);
		REQUIRE(parsedParameter1->GetIPv4Address()[2] == 0x21);
		REQUIRE(parsedParameter1->GetIPv4Address()[3] == 0x2C);

		const auto* parsedParameter2 =
		  reinterpret_cast<const IPv6AddressChunkParameter*>(parsedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parsedParameter2,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parsedParameter2->GetIPv6Address()[0] == 0x23);
		REQUIRE(parsedParameter2->GetIPv6Address()[1] == 0x45);
		REQUIRE(parsedParameter2->GetIPv6Address()[2] == 0x04);
		REQUIRE(parsedParameter2->GetIPv6Address()[3] == 0x25);
		REQUIRE(parsedParameter2->GetIPv6Address()[15] == 0xB5);

		const auto* parsedParameter3 =
		  reinterpret_cast<const CookiePreservativeChunkParameter*>(parsedChunk->GetParameterAt(2));

		checkChunkParameter(
		  /*parameter*/ parsedParameter3,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parsedParameter3->GetLifeSpanIncrement() == 876543210);

		delete parsedChunk;
	}
}

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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 1);

		REQUIRE(chunk->GetInitiateTag() == 287454020);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 4278216311);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 4660);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 22136);
		REQUIRE(chunk->GetInitialTSN() == 2882339074);

		auto* parameter1 = reinterpret_cast<const IPv4AddressChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter1->GetIPv4Address()[0] == 0x02);
		REQUIRE(parameter1->GetIPv4Address()[1] == 0x03);
		REQUIRE(parameter1->GetIPv4Address()[2] == 0x04);
		REQUIRE(parameter1->GetIPv4Address()[3] == 0x05);

		delete chunk;
	}

	SECTION("InitAckChunk::Factory() succeeds")
	{
		auto* chunk = InitAckChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetInitiateTag() == 0);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 0);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 0);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 0);
		REQUIRE(chunk->GetInitialTSN() == 0);

		/* Modify it and add Parameters. */

		chunk->SetInitiateTag(1111111110);
		chunk->SetAdvertisedReceiverWindowCredit(2222222220);
		chunk->SetNumberOfOutboundStreams(1234);
		chunk->SetNumberOfInboundStreams(5678);
		chunk->SetInitialTSN(3333333330);

		auto* parameter1 = reinterpret_cast<IPv4AddressChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::IPV4_ADDRESS));

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer1[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter1->SetIPv4Address(ipBuffer1);
		parameter1->Consolidate();

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 28,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::INIT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 1);

		REQUIRE(chunk->GetInitiateTag() == 1111111110);
		REQUIRE(chunk->GetAdvertisedReceiverWindowCredit() == 2222222220);
		REQUIRE(chunk->GetNumberOfOutboundStreams() == 1234);
		REQUIRE(chunk->GetNumberOfInboundStreams() == 5678);
		REQUIRE(chunk->GetInitialTSN() == 3333333330);

		const auto* addedParameter1 =
		  reinterpret_cast<const IPv4AddressChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ addedParameter1,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(addedParameter1->GetIPv4Address()[0] == 0x0B);
		REQUIRE(addedParameter1->GetIPv4Address()[1] == 0x16);
		REQUIRE(addedParameter1->GetIPv4Address()[2] == 0x21);
		REQUIRE(addedParameter1->GetIPv4Address()[3] == 0x2C);

		delete chunk;
	}
}

SCENARIO("SCTP Hearbeat Request Chunk (4)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("HeartbeatChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:4 (HEARTBEAT), Flags:0b00000000, Length: 22
			// NOTE: Length field must exclude the padding of the last Parameter.
			0x04, 0b00000000, 0x00, 0x16,
			// Parameter 1: Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
			// Parameter 2: Type:49159 (UNKNOWN), Length: 6
			0xC0, 0x07, 0x00, 0x06,
			// Unknown data: 0xABCD, 2 bytes of padding
			0xAB, 0xCD, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = HeartbeatChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		auto* parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		auto* parameter2 = reinterpret_cast<const UnknownChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO),
		  MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		parameter2 = reinterpret_cast<const UnknownChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(clonedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		parameter2 = reinterpret_cast<const UnknownChunkParameter*>(clonedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("HeartbeatChunk::Parse() with incorrect but valid Chunk Length field succeeds")
	{
		// Here the chunk has incorrect Chunk Length field with value 24 instead of
		// 22. It's incorrect because, as per RFC 9260:
		//
		// > The Chunk Length field does not count any chunk padding. However, it
		// > does include any padding of variable-length parameters other than the
		// > last parameter in the chunk. A robust implementation is expected to
		// > accept the chunk whether or not the final padding has been included in
		// > the Chunk Length.

		// clang-format off
		uint8_t buffer[] =
		{
			// Type:4 (HEARTBEAT), Flags:0b00000000, Length: 24
			// NOTE: Length field must exclude the padding of the last Parameter so
			// Length field should be 22 rather than 24. But anyway it's ok.
			0x04, 0b00000000, 0x00, 0x18,
			// Parameter 1: Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
			// Parameter 2: Type:49159 (UNKNOWN), Length: 6
			0xC0, 0x07, 0x00, 0x06,
			// Unknown data: 0xABCD, 2 bytes of padding
			0xAB, 0xCD, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = HeartbeatChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		auto* parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		auto* parameter2 = reinterpret_cast<const UnknownChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(clonedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		parameter2 = reinterpret_cast<const UnknownChunkParameter*>(clonedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("HeartbeatChunk::Factory() succeeds")
	{
		auto* chunk = HeartbeatChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		/* Modify it by adding Chunk Parameters. */

		auto* parameter1 = reinterpret_cast<HeartbeatInfoChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO));

		// Info length is 5 so 3 bytes of padding will be added.
		parameter1->SetInfo(ChunkCustomDataBuffer, 5);
		parameter1->Consolidate();

		// Let's add another HeartbeatInfoChunkParameter (it doesn't make sense but
		// anyway).
		auto* parameter2 = reinterpret_cast<HeartbeatInfoChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO));

		// Info length is 2 so 2 bytes of padding will be added.
		parameter2->SetInfo(ChunkCustomDataBuffer, 2);
		parameter2->Consolidate();

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		const auto* addedParameter1 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ addedParameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(addedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(addedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(addedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(addedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(addedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[6] == 0x00);

		const auto* addedParameter2 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ addedParameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 2);

		REQUIRE(addedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(addedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = HeartbeatChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		const auto* parsedParameter1 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(parsedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parsedParameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(parsedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(parsedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(parsedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(parsedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[6] == 0x00);

		const auto* parsedParameter2 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(parsedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parsedParameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 2);

		REQUIRE(parsedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(parsedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[3] == 0x00);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Hearbeat Acknowledgement Chunk (5)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("HeartbeatAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:5 (HEARTBEAT_ACK), Flags:0b00000000, Length: 22
			// NOTE: Length field must exclude the padding of the last Parameter.
			0x05, 0b00000000, 0x00, 0x16,
			// Parameter 1: Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
			// Parameter 2: Type:49159 (UNKNOWN), Length: 6
			0xC0, 0x07, 0x00, 0x06,
			// Unknown data: 0xABCD, 2 bytes of padding
			0xAB, 0xCD, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = HeartbeatAckChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		auto* parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		auto* parameter2 = reinterpret_cast<const UnknownChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO),
		  MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		parameter2 = reinterpret_cast<const UnknownChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		parameter1 = reinterpret_cast<const HeartbeatInfoChunkParameter*>(clonedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);

		parameter2 = reinterpret_cast<const UnknownChunkParameter*>(clonedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<ChunkParameter::ChunkParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::SKIP_AND_REPORT,
		  /*valueLength*/ 2);

		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// This should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("HeartbeatAckChunk::Factory() succeeds")
	{
		auto* chunk = HeartbeatAckChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		/* Modify it by adding Chunk Parameters. */

		auto* parameter1 = reinterpret_cast<HeartbeatInfoChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO));

		// Info length is 5 so 3 bytes of padding will be added.
		parameter1->SetInfo(ChunkCustomDataBuffer, 5);
		parameter1->Consolidate();

		// Let's add another HeartbeatInfoChunkParameter (it doesn't make sense but
		// anyway).
		auto* parameter2 = reinterpret_cast<HeartbeatInfoChunkParameter*>(
		  chunk->BuildParameterInPlace(ChunkParameter::ChunkParameterType::HEARTBEAT_INFO));

		// Info length is 2 so 2 bytes of padding will be added.
		parameter2->SetInfo(ChunkCustomDataBuffer, 2);
		parameter2->Consolidate();

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		const auto* addedParameter1 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ addedParameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(addedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(addedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(addedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(addedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(addedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[6] == 0x00);

		const auto* addedParameter2 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(chunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ addedParameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 2);

		REQUIRE(addedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(addedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = HeartbeatAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 2);

		const auto* parsedParameter1 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(parsedChunk->GetParameterAt(0));

		checkChunkParameter(
		  /*parameter*/ parsedParameter1,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(parsedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(parsedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(parsedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(parsedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[6] == 0x00);

		const auto* parsedParameter2 =
		  reinterpret_cast<const HeartbeatInfoChunkParameter*>(parsedChunk->GetParameterAt(1));

		checkChunkParameter(
		  /*parameter*/ parsedParameter2,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 2);

		REQUIRE(parsedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(parsedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[3] == 0x00);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Association Chunk (7)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:7 (SHUTDOWN), Flags:0x00000000, Length: 8
			0x07, 0b00000000, 0x00, 0x08,
			// Cumulative TSN Ack: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = ShutdownChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetCumulativeTsnAck(666), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(clonedChunk->GetCumulativeTsnAck() == 0x11223344);

		delete clonedChunk;
	}

	SECTION("ShutdownChunk::Factory() succeeds")
	{
		auto* chunk = ShutdownChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0);

		/* Modify it. */

		chunk->SetCumulativeTsnAck(99887766);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetCumulativeTsnAck() == 99887766);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(parsedChunk->GetCumulativeTsnAck() == 99887766);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Ack Chunk (8)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:8 (SHUTDOWN_ACK), Flags:0x00000000, Length: 4
			0x08, 0b01000000, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = ShutdownAckChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b01000000,
		  /*parametersCount*/ 0);

		delete clonedChunk;
	}

	SECTION("ShutdownAckChunk::Factory() succeeds")
	{
		auto* chunk = ShutdownAckChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Cookie Acknowledgement Chunk (11)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("CookieAckChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:11 (COOKIE_ACK), Flags:0x00000001, T: 1, Length: 4
			0x0B, 0b00000101, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA
		};
		// clang-format on

		auto* chunk = CookieAckChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*parametersCount*/ 0);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*parametersCount*/ 0);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000101,
		  /*parametersCount*/ 0);

		delete clonedChunk;
	}

	SECTION("CookieAckChunk::Factory() succeeds")
	{
		auto* chunk = CookieAckChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		/* Parse itself and compare. */

		auto* parsedChunk = CookieAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::COOKIE_ACK,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Complete Chunk (14)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownCompleteChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:8 (SHUTDOWN_COMPLETE), Flags:0x00000001, T: 1, Length: 4
			0x0E, 0b00000001, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		auto* chunk = ShutdownCompleteChunk::Parse(buffer, sizeof(buffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetT(false), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(clonedChunk->GetT() == true);

		delete clonedChunk;
	}

	SECTION("ShutdownCompleteChunk::Factory() succeeds")
	{
		auto* chunk = ShutdownCompleteChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == false);

		/* Modify it. */

		chunk->SetT(true);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->GetT() == true);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownCompleteChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000001,
		  /*parametersCount*/ 0);

		REQUIRE(parsedChunk->GetT() == true);

		delete parsedChunk;
	}
}

SCENARIO("SCTP Unknown Chunk (238)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnknownChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:0xEE (UNKNOWN), Flags: 0b1100, Length: 7
			0xEE, 0b10001100, 0x00, 0x07,
			// Unknown data: 0xAABBCC, 1 byte of padding
			0xAA, 0xBB, 0xCC, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* chunk = UnknownChunk::Parse(buffer, sizeof(buffer));

		// NOTE: Chunk Type is 0xEE (0b11101110) so first 2 bits are 11, meaning
		// that the action to take if we receive this Chunk Type is SKIP_AND_REPORT.

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->HasUnknownValue() == true);
		REQUIRE(chunk->GetUnknownValueLength() == 3);
		REQUIRE(chunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownValue()[2] == 0xCC);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*parametersCount*/ 0);

		REQUIRE(chunk->HasUnknownValue() == true);
		REQUIRE(chunk->GetUnknownValueLength() == 3);
		REQUIRE(chunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownValue()[2] == 0xCC);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT,
		  /*flags*/ 0b10001100,
		  /*parametersCount*/ 0);

		REQUIRE(clonedChunk->HasUnknownValue() == true);
		REQUIRE(clonedChunk->GetUnknownValueLength() == 3);
		REQUIRE(clonedChunk->GetUnknownValue()[0] == 0xAA);
		REQUIRE(clonedChunk->GetUnknownValue()[1] == 0xBB);
		REQUIRE(clonedChunk->GetUnknownValue()[2] == 0xCC);

		delete clonedChunk;
	}
}

void resetBuffers()
{
	std::memset(ChunkFactoryBuffer, 0xAA, sizeof(ChunkFactoryBuffer));
	std::memset(ChunkSerializeBuffer, 0xBB, sizeof(ChunkSerializeBuffer));
	std::memset(ChunkCloneBuffer, 0xCC, sizeof(ChunkCloneBuffer));
	std::memset(ChunkCustomDataBuffer, 0xDD, sizeof(ChunkCustomDataBuffer));
	std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

	ChunkCustomDataBuffer[0] = 0x00;
	ChunkCustomDataBuffer[1] = 0x01;
	ChunkCustomDataBuffer[2] = 0x02;
	ChunkCustomDataBuffer[3] = 0x03;
	ChunkCustomDataBuffer[4] = 0x04;
	ChunkCustomDataBuffer[5] = 0x05;
	ChunkCustomDataBuffer[6] = 0x06;
	ChunkCustomDataBuffer[7] = 0x07;
}

void checkChunk(
  const Chunk* chunk,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  Chunk::ActionForUnknownChunkType actionForUnknownChunkType,
  uint8_t flags,
  size_t parametersCount)
{
	REQUIRE(chunk);
	REQUIRE(chunk->GetBuffer() == buffer);
	REQUIRE(chunk->GetBufferLength() == bufferLength);
	REQUIRE(chunk->GetLength() == length);
	REQUIRE(chunk->IsFrozen() == frozen);
	REQUIRE(chunk->GetType() == chunkType);
	REQUIRE(chunk->HasUnknownType() == unknownType);
	REQUIRE(chunk->GetActionForUnknownChunkType() == actionForUnknownChunkType);
	REQUIRE(chunk->GetFlags() == flags);
	REQUIRE(chunk->HasParameters() == parametersCount > 0);
	REQUIRE(chunk->GetParametersCount() == parametersCount);
	REQUIRE(chunk->GetParameterAt(parametersCount) == nullptr);
	REQUIRE(helpers::areBuffersEqual(chunk->GetBuffer(), chunk->GetLength(), buffer, length) == true);

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(const_cast<Chunk*>(chunk)->Serialize(ThrowBuffer, length - 1), MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(chunk->Clone(ThrowBuffer, length - 1), MediaSoupError);
}

static void checkChunkParameter(
  const ChunkParameter* parameter,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength)
{
	REQUIRE(parameter);
	REQUIRE(parameter->GetBufferLength() == bufferLength);
	REQUIRE(parameter->GetLength() == length);
	REQUIRE(parameter->IsFrozen() == frozen);
	REQUIRE(parameter->GetType() == parameterType);
	REQUIRE(parameter->HasUnknownType() == unknownType);
	REQUIRE(parameter->GetActionForUnknownChunkParameterType() == actionForUnknownParameterType);
	REQUIRE(parameter->HasValue() == valueLength > 0);
	REQUIRE(parameter->GetValueLength() == valueLength);

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, length - 1), MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, length - 1), MediaSoupError);
}
