#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Supported Extensions Parameter (32776)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("SupportedExtensionsParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:32776 (SUPPORTED_EXTENSIONS), Length: 7
			0x80, 0x08, 0x00, 0x07,
			// Chunk Type 1: RE_CONFIG (0x82), Chunk Type 2: ECNE (0x0C),
			// Chunk Type 3: UNKNOWN (0x42), 1 byte of padding
			0x82, 0x0C, 0x42, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = SupportedExtensionsParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(parameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == Chunk::ChunkType::ECNE);
		REQUIRE(parameter->GetChunkTypeAt(2) == static_cast<Chunk::ChunkType>(0x42));

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->AddChunkType(Chunk::ChunkType::COOKIE_ACK), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(parameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == Chunk::ChunkType::ECNE);
		REQUIRE(parameter->GetChunkTypeAt(2) == static_cast<Chunk::ChunkType>(0x42));

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(clonedParameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(clonedParameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(clonedParameter->GetChunkTypeAt(1) == Chunk::ChunkType::ECNE);
		REQUIRE(clonedParameter->GetChunkTypeAt(2) == static_cast<Chunk::ChunkType>(0x42));

		delete clonedParameter;
	}

	SECTION("SupportedExtensionsParameter::Factory() succeeds")
	{
		auto* parameter = SupportedExtensionsParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 0);

		/* Modify it. */

		parameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);
		parameter->AddChunkType(Chunk::ChunkType::CWR);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 2);
		REQUIRE(parameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == Chunk::ChunkType::CWR);

		parameter->AddChunkType(Chunk::ChunkType::OPERATION_ERROR);
		parameter->AddChunkType(Chunk::ChunkType::COOKIE_ACK);
		parameter->AddChunkType(static_cast<Chunk::ChunkType>(99));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 5);
		REQUIRE(parameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == Chunk::ChunkType::CWR);
		REQUIRE(parameter->GetChunkTypeAt(2) == Chunk::ChunkType::OPERATION_ERROR);
		REQUIRE(parameter->GetChunkTypeAt(3) == Chunk::ChunkType::COOKIE_ACK);
		REQUIRE(parameter->GetChunkTypeAt(4) == static_cast<Chunk::ChunkType>(99));

		/* Parse itself and compare. */

		auto* parsedParameter =
		  SupportedExtensionsParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parsedParameter->GetNumberOfChunkTypes() == 5);
		REQUIRE(parsedParameter->GetChunkTypeAt(0) == Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parsedParameter->GetChunkTypeAt(1) == Chunk::ChunkType::CWR);
		REQUIRE(parsedParameter->GetChunkTypeAt(2) == Chunk::ChunkType::OPERATION_ERROR);
		REQUIRE(parsedParameter->GetChunkTypeAt(3) == Chunk::ChunkType::COOKIE_ACK);
		REQUIRE(parsedParameter->GetChunkTypeAt(4) == static_cast<Chunk::ChunkType>(99));

		delete parsedParameter;
	}
}
