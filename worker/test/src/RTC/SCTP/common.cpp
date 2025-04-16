#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp" // in worker/test/include/
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset

using namespace RTC::SCTP;

thread_local uint8_t FactoryBuffer[];
thread_local uint8_t SerializeBuffer[];
thread_local uint8_t CloneBuffer[];
thread_local uint8_t DataBuffer[];
thread_local uint8_t ThrowBuffer[];

void resetBuffers()
{
	std::memset(FactoryBuffer, 0xAA, sizeof(FactoryBuffer));
	std::memset(SerializeBuffer, 0xBB, sizeof(SerializeBuffer));
	std::memset(CloneBuffer, 0xCC, sizeof(CloneBuffer));
	std::memset(DataBuffer, 0xDD, sizeof(DataBuffer));
	std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

	DataBuffer[0] = 0x00;
	DataBuffer[1] = 0x01;
	DataBuffer[2] = 0x02;
	DataBuffer[3] = 0x03;
	DataBuffer[4] = 0x04;
	DataBuffer[5] = 0x05;
	DataBuffer[6] = 0x06;
	DataBuffer[7] = 0x07;
}

void checkPacket(
  const Packet* packet,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  uint16_t sourcePort,
  uint16_t destinationPort,
  uint32_t verificationTag,
  uint32_t checksum,
  size_t chunksCount)
{
	REQUIRE(packet);
	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == bufferLength);
	REQUIRE(packet->GetLength() == length);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->IsFrozen() == frozen);
	REQUIRE(packet->GetSourcePort() == sourcePort);
	REQUIRE(packet->GetDestinationPort() == destinationPort);
	REQUIRE(packet->GetVerificationTag() == verificationTag);
	REQUIRE(packet->GetChecksum() == checksum);
	REQUIRE(packet->HasChunks() == chunksCount > 0);
	REQUIRE(packet->GetChunksCount() == chunksCount);
	REQUIRE(packet->GetChunkAt(chunksCount) == nullptr);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, length) == true);

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(const_cast<Packet*>(packet)->Serialize(ThrowBuffer, length - 1), MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(packet->Clone(ThrowBuffer, length - 1), MediaSoupError);
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

	if (buffer)
	{
		REQUIRE(chunk->GetBuffer() == buffer);
	}

	REQUIRE(chunk->GetBufferLength() == bufferLength);
	REQUIRE(chunk->GetLength() == length);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(chunk->GetLength()) == true);
	REQUIRE(chunk->IsFrozen() == frozen);
	REQUIRE(chunk->GetType() == chunkType);
	REQUIRE(chunk->HasUnknownType() == unknownType);
	REQUIRE(chunk->GetActionForUnknownChunkType() == actionForUnknownChunkType);
	REQUIRE(chunk->GetFlags() == flags);
	REQUIRE(chunk->HasParameters() == parametersCount > 0);
	REQUIRE(chunk->GetParametersCount() == parametersCount);
	REQUIRE(chunk->GetParameterAt(parametersCount) == nullptr);

	if (buffer)
	{
		REQUIRE(helpers::areBuffersEqual(chunk->GetBuffer(), chunk->GetLength(), buffer, length) == true);
	}

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(const_cast<Chunk*>(chunk)->Serialize(ThrowBuffer, length - 1), MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(chunk->Clone(ThrowBuffer, length - 1), MediaSoupError);
}

void checkChunk(
  const Chunk* chunk,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  Chunk::ActionForUnknownChunkType actionForUnknownChunkType,
  uint8_t flags,
  size_t parametersCount)
{
	checkChunk(
	  /*chunk*/ chunk,
	  /*buffer*/ nullptr,
	  /*bufferLength*/ bufferLength,
	  /*length*/ length,
	  /*frozen*/ frozen,
	  /*chunkType*/ chunkType,
	  /*unknownType*/ unknownType,
	  /*actionForUnknownChunkType*/ actionForUnknownChunkType,
	  /*flags*/ flags,
	  /*parametersCount*/ parametersCount);
}

void checkChunkParameter(
  const ChunkParameter* parameter,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength)
{
	REQUIRE(parameter);

	if (buffer)
	{
		REQUIRE(parameter->GetBuffer() == buffer);
	}

	REQUIRE(parameter->GetBufferLength() == bufferLength);
	REQUIRE(parameter->GetLength() == length);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(parameter->GetLength()) == true);
	REQUIRE(parameter->IsFrozen() == frozen);
	REQUIRE(parameter->GetType() == parameterType);
	REQUIRE(parameter->HasUnknownType() == unknownType);
	REQUIRE(parameter->GetActionForUnknownChunkParameterType() == actionForUnknownParameterType);
	REQUIRE(parameter->HasValue() == valueLength > 0);
	REQUIRE(parameter->GetValueLength() == valueLength);

	if (buffer)
	{
		REQUIRE(
		  helpers::areBuffersEqual(parameter->GetBuffer(), parameter->GetLength(), buffer, length) ==
		  true);
	}

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, length - 1), MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, length - 1), MediaSoupError);
}

void checkChunkParameter(
  const ChunkParameter* parameter,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength)
{
	checkChunkParameter(
	  /*parameter*/ parameter,
	  /*buffer*/ nullptr,
	  /*bufferLength*/ bufferLength,
	  /*length*/ length,
	  /*frozen*/ frozen,
	  /*parameterType*/ parameterType,
	  /*unknownType*/ unknownType,
	  /*actionForUnknownParameterType*/ actionForUnknownParameterType,
	  /*valueLength*/ valueLength);
}
