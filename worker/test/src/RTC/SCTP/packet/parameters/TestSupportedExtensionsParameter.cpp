#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Supported Extensions Parameter (32776)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("SupportedExtensionsParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* parameter = RTC::SCTP::SupportedExtensionsParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(parameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::ECNE);
		REQUIRE(parameter->GetChunkTypeAt(2) == static_cast<RTC::SCTP::Chunk::ChunkType>(0x42));
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::ECNE) == true);
		REQUIRE(parameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(0x42)) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA) == false);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(parameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::ECNE);
		REQUIRE(parameter->GetChunkTypeAt(2) == static_cast<RTC::SCTP::Chunk::ChunkType>(0x42));
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::ECNE) == true);
		REQUIRE(parameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(0x42)) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA) == false);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(clonedParameter->GetNumberOfChunkTypes() == 3);
		REQUIRE(clonedParameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(clonedParameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::ECNE);
		REQUIRE(clonedParameter->GetChunkTypeAt(2) == static_cast<RTC::SCTP::Chunk::ChunkType>(0x42));
		REQUIRE(clonedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(clonedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::ECNE) == true);
		REQUIRE(
		  clonedParameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(0x42)) == true);
		REQUIRE(clonedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::I_DATA) == false);

		delete clonedParameter;
	}

	SECTION("SupportedExtensionsParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::SupportedExtensionsParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 0);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == false);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::ECNE) == false);
		REQUIRE(parameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(0x42)) == false);

		/* Modify it. */

		parameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		parameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::CWR);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 2);
		REQUIRE(parameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::CWR);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::CWR) == true);

		parameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR);
		parameter->AddChunkType(RTC::SCTP::Chunk::ChunkType::COOKIE_ACK);
		parameter->AddChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(99));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parameter->GetNumberOfChunkTypes() == 5);
		REQUIRE(parameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::CWR);
		REQUIRE(parameter->GetChunkTypeAt(2) == RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR);
		REQUIRE(parameter->GetChunkTypeAt(3) == RTC::SCTP::Chunk::ChunkType::COOKIE_ACK);
		REQUIRE(parameter->GetChunkTypeAt(4) == static_cast<RTC::SCTP::Chunk::ChunkType>(99));
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::CWR) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR) == true);
		REQUIRE(parameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::COOKIE_ACK) == true);
		REQUIRE(parameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(99)) == true);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::SupportedExtensionsParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::SKIP);

		REQUIRE(parsedParameter->GetNumberOfChunkTypes() == 5);
		REQUIRE(parsedParameter->GetChunkTypeAt(0) == RTC::SCTP::Chunk::ChunkType::RE_CONFIG);
		REQUIRE(parsedParameter->GetChunkTypeAt(1) == RTC::SCTP::Chunk::ChunkType::CWR);
		REQUIRE(parsedParameter->GetChunkTypeAt(2) == RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR);
		REQUIRE(parsedParameter->GetChunkTypeAt(3) == RTC::SCTP::Chunk::ChunkType::COOKIE_ACK);
		REQUIRE(parsedParameter->GetChunkTypeAt(4) == static_cast<RTC::SCTP::Chunk::ChunkType>(99));
		REQUIRE(parsedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::RE_CONFIG) == true);
		REQUIRE(parsedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::CWR) == true);
		REQUIRE(parsedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR) == true);
		REQUIRE(parsedParameter->IncludesChunkType(RTC::SCTP::Chunk::ChunkType::COOKIE_ACK) == true);
		REQUIRE(parsedParameter->IncludesChunkType(static_cast<RTC::SCTP::Chunk::ChunkType>(99)) == true);

		delete parsedParameter;
	}
}
