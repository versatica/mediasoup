#ifndef MS_TEST_RTC_SCTP_COMMON_HPP
#define MS_TEST_RTC_SCTP_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"            // IWYU pragma: export
#include "Utils.hpp"                      // IWYU pragma: export
#include "testHelpers.hpp"                // IWYU pragma: export in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"      // IWYU pragma: export
#include "RTC/SCTP/packet/ErrorCause.hpp" // IWYU pragma: export
#include "RTC/SCTP/packet/Packet.hpp"     // IWYU pragma: export
#include "RTC/SCTP/packet/Parameter.hpp"  // IWYU pragma: export
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp" // IWYU pragma: export
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"             // IWYU pragma: export
#include <catch2/catch_test_macros.hpp>                                      // IWYU pragma: export

using namespace RTC::SCTP;

namespace RTC
{
	namespace SCTP
	{
		// NOTE: We need to declare them here with `extern` and then define them in
		// common.cpp.
		extern thread_local uint8_t FactoryBuffer[66661];
		extern thread_local uint8_t SerializeBuffer[66662];
		extern thread_local uint8_t CloneBuffer[66663];
		extern thread_local uint8_t DataBuffer[66664];
		extern thread_local uint8_t ThrowBuffer[66665];

		void ResetBuffers();
	} // namespace SCTP
} // namespace RTC

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_SCTP_PACKET(/*const Packet**/ packet,                                                \
                          /*const uint8_t**/ buffer,                                               \
                          /*size_t*/ bufferLength,                                                 \
                          /*size_t*/ length,                                                       \
                          /*uint16_t*/ sourcePort,                                                 \
                          /*uint16_t*/ destinationPort,                                            \
                          /*uint32_t*/ verificationTag,                                            \
                          /*uint32_t*/ checksum,                                                   \
                          /*bool*/ hasValidCrc32cChecksum,                                         \
                          /*size_t*/ chunksCount)                                                  \
	do                                                                                               \
	{                                                                                                \
		REQUIRE(packet);                                                                               \
		REQUIRE(packet->GetBuffer() != nullptr);                                                       \
		REQUIRE(packet->GetBuffer() == buffer);                                                        \
		REQUIRE(packet->GetBufferLength() != 0);                                                       \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                            \
		REQUIRE(packet->GetLength() != 0);                                                             \
		REQUIRE(packet->GetLength() == length);                                                        \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);                           \
		REQUIRE(packet->GetSourcePort() == sourcePort);                                                \
		REQUIRE(packet->GetDestinationPort() == destinationPort);                                      \
		REQUIRE(packet->GetVerificationTag() == verificationTag);                                      \
		REQUIRE(packet->GetChecksum() == checksum);                                                    \
		REQUIRE(packet->ValidateCRC32cChecksum() == hasValidCrc32cChecksum);                           \
		REQUIRE(packet->GetChunksCount() == chunksCount);                                              \
		REQUIRE(packet->HasChunks() == (chunksCount > 0));                                             \
		REQUIRE(packet->GetChunkAt(chunksCount) == nullptr);                                           \
		REQUIRE(                                                                                       \
		  helpers::AreBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, length) == true); \
		REQUIRE_THROWS_AS(                                                                             \
		  const_cast<Packet*>(packet)->Serialize(ThrowBuffer, length - 1), MediaSoupError);            \
		REQUIRE_THROWS_AS(packet->Clone(ThrowBuffer, length - 1), MediaSoupError);                     \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_SCTP_CHUNK(/*const Chunk**/ chunk,                                                     \
                         /*uint8_t**/ buffer,                                                        \
                         /*size_t*/ bufferLength,                                                    \
                         /*size_t*/ length,                                                          \
                         /*Chunk::ChunkType*/ chunkType,                                             \
                         /*bool*/ unknownType,                                                       \
                         /*Chunk::ActionForUnknownChunkType*/ actionForUnknownChunkType,             \
                         /*uint8_t*/ flags,                                                          \
                         /*bool*/ canHaveParameters,                                                 \
                         /*size_t*/ parametersCount,                                                 \
                         /*bool*/ canHaveErrorCauses,                                                \
                         /*size_t*/ errorCausesCount)                                                \
	do                                                                                                 \
	{                                                                                                  \
		REQUIRE(chunk);                                                                                  \
		REQUIRE(chunk->GetBuffer() != nullptr);                                                          \
		if (buffer)                                                                                      \
		{                                                                                                \
			REQUIRE(chunk->GetBuffer() == buffer);                                                         \
		}                                                                                                \
		REQUIRE(chunk->GetBufferLength() != 0);                                                          \
		REQUIRE(chunk->GetBufferLength() == bufferLength);                                               \
		REQUIRE(chunk->GetLength() != 0);                                                                \
		REQUIRE(chunk->GetLength() == length);                                                           \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(chunk->GetLength()) == true);                              \
		REQUIRE(chunk->GetType() == chunkType);                                                          \
		REQUIRE(chunk->HasUnknownType() == unknownType);                                                 \
		REQUIRE(chunk->GetActionForUnknownChunkType() == actionForUnknownChunkType);                     \
		REQUIRE(chunk->GetFlags() == flags);                                                             \
		REQUIRE(chunk->CanHaveParameters() == canHaveParameters);                                        \
		if (!canHaveParameters)                                                                          \
		{                                                                                                \
			REQUIRE_THROWS_AS(                                                                             \
			  const_cast<Chunk*>(reinterpret_cast<const Chunk*>(chunk))                                    \
			    ->BuildParameterInPlace<HeartbeatInfoParameter>(),                                         \
			  MediaSoupError);                                                                             \
		}                                                                                                \
		REQUIRE(chunk->GetParametersCount() == parametersCount);                                         \
		REQUIRE(chunk->HasParameters() == (parametersCount > 0));                                        \
		REQUIRE(chunk->GetParameterAt(parametersCount) == nullptr);                                      \
		REQUIRE(chunk->CanHaveErrorCauses() == canHaveErrorCauses);                                      \
		if (!canHaveErrorCauses)                                                                         \
		{                                                                                                \
			REQUIRE_THROWS_AS(                                                                             \
			  const_cast<Chunk*>(reinterpret_cast<const Chunk*>(chunk))                                    \
			    ->BuildErrorCauseInPlace<InvalidStreamIdentifierErrorCause>(),                             \
			  MediaSoupError);                                                                             \
		}                                                                                                \
		REQUIRE(chunk->GetErrorCausesCount() == errorCausesCount);                                       \
		REQUIRE(chunk->HasErrorCauses() == (errorCausesCount > 0));                                      \
		REQUIRE(chunk->GetErrorCauseAt(errorCausesCount) == nullptr);                                    \
		if (buffer)                                                                                      \
		{                                                                                                \
			REQUIRE(                                                                                       \
			  helpers::AreBuffersEqual(chunk->GetBuffer(), chunk->GetLength(), buffer, length) == true);   \
		}                                                                                                \
		REQUIRE_THROWS_AS(                                                                               \
		  const_cast<Chunk*>(reinterpret_cast<const Chunk*>(chunk))->Serialize(ThrowBuffer, length - 1), \
		  MediaSoupError);                                                                               \
		REQUIRE_THROWS_AS(chunk->Clone(ThrowBuffer, length - 1), MediaSoupError);                        \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_SCTP_PARAMETER(                                                                       \
  /*const Parameter**/ parameter,                                                                   \
  /*const uint8_t**/ buffer,                                                                        \
  /*size_t*/ bufferLength,                                                                          \
  /*size_t*/ length,                                                                                \
  /*Parameter::ParameterType*/ parameterType,                                                       \
  /*bool*/ unknownType,                                                                             \
  /*Parameter::ActionForUnknownParameterType*/ actionForUnknownParameterType)                       \
	do                                                                                                \
	{                                                                                                 \
		REQUIRE(parameter);                                                                             \
		REQUIRE(parameter->GetBuffer() != nullptr);                                                     \
		if (buffer)                                                                                     \
		{                                                                                               \
			REQUIRE(parameter->GetBuffer() == buffer);                                                    \
		}                                                                                               \
		REQUIRE(parameter->GetBufferLength() != 0);                                                     \
		REQUIRE(parameter->GetBufferLength() == bufferLength);                                          \
		REQUIRE(parameter->GetLength() != 0);                                                           \
		REQUIRE(parameter->GetLength() == length);                                                      \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(parameter->GetLength()) == true);                         \
		REQUIRE(parameter->GetType() == parameterType);                                                 \
		REQUIRE(parameter->HasUnknownType() == unknownType);                                            \
		REQUIRE(parameter->GetActionForUnknownParameterType() == actionForUnknownParameterType);        \
		if (buffer)                                                                                     \
		{                                                                                               \
			REQUIRE(                                                                                      \
			  helpers::AreBuffersEqual(parameter->GetBuffer(), parameter->GetLength(), buffer, length) == \
			  true);                                                                                      \
		}                                                                                               \
		REQUIRE_THROWS_AS(                                                                              \
		  const_cast<Parameter*>(reinterpret_cast<const Parameter*>(parameter))                         \
		    ->Serialize(ThrowBuffer, length - 1),                                                       \
		  MediaSoupError);                                                                              \
		REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, length - 1), MediaSoupError);                   \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_SCTP_ERROR_CAUSE(/*const ErrorCause**/ errorCause,                                   \
                               /*const uint8_t**/ buffer,                                          \
                               /*size_t*/ bufferLength,                                            \
                               /*size_t*/ length,                                                  \
                               /*ErrorCause::ErrorCauseCode*/ causeCode,                           \
                               /*bool*/ unknownCode)                                               \
	do                                                                                               \
	{                                                                                                \
		REQUIRE(errorCause);                                                                           \
		REQUIRE(errorCause->GetBuffer() != nullptr);                                                   \
		if (buffer)                                                                                    \
		{                                                                                              \
			REQUIRE(errorCause->GetBuffer() == buffer);                                                  \
		}                                                                                              \
		REQUIRE(errorCause->GetBufferLength() != 0);                                                   \
		REQUIRE(errorCause->GetBufferLength() == bufferLength);                                        \
		REQUIRE(errorCause->GetLength() != 0);                                                         \
		REQUIRE(errorCause->GetLength() == length);                                                    \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(errorCause->GetLength()) == true);                       \
		REQUIRE(errorCause->GetCode() == causeCode);                                                   \
		REQUIRE(errorCause->HasUnknownCode() == unknownCode);                                          \
		if (buffer)                                                                                    \
		{                                                                                              \
			REQUIRE(                                                                                     \
			  helpers::AreBuffersEqual(                                                                  \
			    errorCause->GetBuffer(), errorCause->GetLength(), buffer, length) == true);              \
		}                                                                                              \
		REQUIRE_THROWS_AS(                                                                             \
		  const_cast<ErrorCause*>(reinterpret_cast<const ErrorCause*>(errorCause))                     \
		    ->Serialize(ThrowBuffer, length - 1),                                                      \
		  MediaSoupError);                                                                             \
		REQUIRE_THROWS_AS(errorCause->Clone(ThrowBuffer, length - 1), MediaSoupError);                 \
	} while (false)

#endif
