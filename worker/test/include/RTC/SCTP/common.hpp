#ifndef MS_TEST_RTC_SCTP_CHUNKS_HELPERS_HPP
#define MS_TEST_RTC_SCTP_CHUNKS_HELPERS_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

using namespace RTC::SCTP;

// NOTE: We need to declare them here with `extern` and then define them in
// helpers.cpp.
extern thread_local uint8_t FactoryBuffer[66661];
extern thread_local uint8_t SerializeBuffer[66662];
extern thread_local uint8_t CloneBuffer[66663];
extern thread_local uint8_t DataBuffer[66664];
extern thread_local uint8_t ThrowBuffer[66665];

void resetBuffers();

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
  size_t parametersCount);

void checkChunkParameter(
  const ChunkParameter* parameter,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength);

/**
 * This is the same as the previous function but doesn't include the buffer.
 * Useful when obtaining the Chunk Parameters from Chunks, so their exact
 * location in a buffer is irrelevant for testing purposes.
 */
void checkChunkParameter(
  const ChunkParameter* parameter,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength);

#endif
