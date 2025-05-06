#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Hearbeat Request Chunk (4)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("HeartbeatRequestChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:4 (HEARTBEAT_REQUEST), Flags:0b00000000, Length: 22
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

		auto* chunk = HeartbeatRequestChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* parameter1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->HasInfo() == true);
		REQUIRE(parameter1->GetInfoLength() == 7);
		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter1->GetInfo()[7] == 0x00);

		auto* parameter2 = reinterpret_cast<const UnknownParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter2->HasUnknownValue() == true);
		REQUIRE(parameter2->GetUnknownValueLength() == 2);
		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// These should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->BuildParameterInPlace<HeartbeatInfoParameter>(), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->HasInfo() == true);
		REQUIRE(parameter1->GetInfoLength() == 7);
		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter1->GetInfo()[7] == 0x00);

		parameter2 = reinterpret_cast<const UnknownParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter2->HasUnknownValue() == true);
		REQUIRE(parameter2->GetUnknownValueLength() == 2);
		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// These should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter1 = reinterpret_cast<const HeartbeatInfoParameter*>(clonedChunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->HasInfo() == true);
		REQUIRE(parameter1->GetInfoLength() == 7);
		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter1->GetInfo()[7] == 0x00);

		parameter2 = reinterpret_cast<const UnknownParameter*>(clonedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter2->HasUnknownValue() == true);
		REQUIRE(parameter2->GetUnknownValueLength() == 2);
		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// These should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("HeartbeatRequestChunk::Parse() with incorrect but valid Chunk Length field succeeds")
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
			// Type:4 (HEARTBEAT_REQUEST), Flags:0b00000000, Length: 24
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

		auto* chunk = HeartbeatRequestChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		auto* parameter1 = reinterpret_cast<const HeartbeatInfoParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->HasInfo() == true);
		REQUIRE(parameter1->GetInfoLength() == 7);
		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter1->GetInfo()[7] == 0x00);

		auto* parameter2 = reinterpret_cast<const UnknownParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter2->HasUnknownValue() == true);
		REQUIRE(parameter2->GetUnknownValueLength() == 2);
		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// These should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		parameter1 = reinterpret_cast<const HeartbeatInfoParameter*>(clonedChunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter1->HasInfo() == true);
		REQUIRE(parameter1->GetInfoLength() == 7);
		REQUIRE(parameter1->GetInfo()[0] == 0x11);
		REQUIRE(parameter1->GetInfo()[1] == 0x22);
		REQUIRE(parameter1->GetInfo()[2] == 0x33);
		REQUIRE(parameter1->GetInfo()[3] == 0x44);
		REQUIRE(parameter1->GetInfo()[4] == 0x55);
		REQUIRE(parameter1->GetInfo()[5] == 0x66);
		REQUIRE(parameter1->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter1->GetInfo()[7] == 0x00);

		parameter2 = reinterpret_cast<const UnknownParameter*>(clonedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ static_cast<Parameter::ParameterType>(49159),
		  /*unknownType*/ true,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP_AND_REPORT);

		REQUIRE(parameter2->HasUnknownValue() == true);
		REQUIRE(parameter2->GetUnknownValueLength() == 2);
		REQUIRE(parameter2->GetUnknownValue()[0] == 0xAB);
		REQUIRE(parameter2->GetUnknownValue()[1] == 0xCD);
		// These should be padding.
		REQUIRE(parameter2->GetUnknownValue()[2] == 0x00);
		REQUIRE(parameter2->GetUnknownValue()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("HeartbeatRequestChunk::Factory() succeeds")
	{
		auto* chunk = HeartbeatRequestChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		/* Modify it by adding Parameters. */

		auto* parameter1 = chunk->BuildParameterInPlace<HeartbeatInfoParameter>();

		// Info length is 5 so 3 bytes of padding will be added.
		parameter1->SetInfo(DataBuffer, 5);
		parameter1->Consolidate();

		// Let's add another HeartbeatInfoParameter (it doesn't make sense but
		// anyway).
		auto* parameter2 = chunk->BuildParameterInPlace<HeartbeatInfoParameter>();

		// Info length is 2 so 2 bytes of padding will be added.
		parameter2->SetInfo(DataBuffer, 2);
		parameter2->Consolidate();

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		const auto* addedParameter1 =
		  reinterpret_cast<const HeartbeatInfoParameter*>(chunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ addedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter1->HasInfo() == true);
		REQUIRE(addedParameter1->GetInfoLength() == 5);
		REQUIRE(addedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(addedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(addedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(addedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(addedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(addedParameter1->GetInfo()[6] == 0x00);

		const auto* addedParameter2 =
		  reinterpret_cast<const HeartbeatInfoParameter*>(chunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ addedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(addedParameter2->HasInfo() == true);
		REQUIRE(addedParameter2->GetInfoLength() == 2);
		REQUIRE(addedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(addedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(addedParameter2->GetInfo()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = HeartbeatRequestChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*length*/ 4 + (4 + 5 + 3) + (4 + 2 + 2),
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::HEARTBEAT_REQUEST,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ true,
		  /*parametersCount*/ 2,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		const auto* parsedParameter1 =
		  reinterpret_cast<const HeartbeatInfoParameter*>(parsedChunk->GetParameterAt(0));

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter1,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter1->HasInfo() == true);
		REQUIRE(parsedParameter1->GetInfoLength() == 5);
		REQUIRE(parsedParameter1->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[1] == 0x01);
		REQUIRE(parsedParameter1->GetInfo()[2] == 0x02);
		REQUIRE(parsedParameter1->GetInfo()[3] == 0x03);
		REQUIRE(parsedParameter1->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedParameter1->GetInfo()[5] == 0x00);
		REQUIRE(parsedParameter1->GetInfo()[6] == 0x00);

		const auto* parsedParameter2 =
		  reinterpret_cast<const HeartbeatInfoParameter*>(parsedChunk->GetParameterAt(1));

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter2,
		  /*buffer*/ nullptr,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter2->HasInfo() == true);
		REQUIRE(parsedParameter2->GetInfoLength() == 2);
		REQUIRE(parsedParameter2->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[1] == 0x01);
		// These should be padding.
		REQUIRE(parsedParameter2->GetInfo()[2] == 0x00);
		REQUIRE(parsedParameter2->GetInfo()[3] == 0x00);

		delete parsedChunk;
	}
}
