#define MS_CLASS "RTC::SCTP::ReConfigChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ReConfigChunk* ReConfigChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::RE_CONFIG)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return ReConfigChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		ReConfigChunk* ReConfigChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new ReConfigChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::RE_CONFIG, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum ReConfigChunk length.

			return chunk;
		}

		ReConfigChunk* ReConfigChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			auto* chunk = new ReConfigChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Parse Parameters.
			if (!chunk->ParseParameters())
			{
				MS_WARN_DEV("failed to parse Parameters");

				delete chunk;
				return nullptr;
			}

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		ReConfigChunk::ReConfigChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		ReConfigChunk::~ReConfigChunk()
		{
			MS_TRACE();
		}

		void ReConfigChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ReConfigChunk>");
			DumpCommon(indentation);
			DumpParameters(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::ReConfigChunk>");
		}

		ReConfigChunk* ReConfigChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new ReConfigChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		ReConfigChunk* ReConfigChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new ReConfigChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
