#define MS_CLASS "RTC::SCTP::ShutdownCompleteChunk"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/ShutdownCompleteChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ShutdownCompleteChunk* ShutdownCompleteChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::SHUTDOWN_COMPLETE)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			if (chunkLength != ShutdownCompleteChunk::ShutdownCompleteChunkLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "ShutdownCompleteChunk Length field must be %zu",
				  ShutdownCompleteChunk::ShutdownCompleteChunkLength);

				return nullptr;
			}

			auto* chunk = new ShutdownCompleteChunk(buffer, bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		ShutdownCompleteChunk* ShutdownCompleteChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ShutdownCompleteChunk::ShutdownCompleteChunkLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new ShutdownCompleteChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::SHUTDOWN_COMPLETE, 0, ShutdownCompleteChunk::ShutdownCompleteChunkLength);

			// No need to invoke SetLength() since constructor invoked it with
			// ShutdownCompleteChunk fixed length.

			return chunk;
		}

		/* Instance methods. */

		ShutdownCompleteChunk::ShutdownCompleteChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(ShutdownCompleteChunk::ShutdownCompleteChunkLength);
		}

		ShutdownCompleteChunk::~ShutdownCompleteChunk()
		{
			MS_TRACE();
		}

		void ShutdownCompleteChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ShutdownCompleteChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::ShutdownCompleteChunk>");
		}

		ShutdownCompleteChunk* ShutdownCompleteChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new ShutdownCompleteChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void ShutdownCompleteChunk::SetT(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit0(flag);
		}

		ShutdownCompleteChunk* ShutdownCompleteChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new ShutdownCompleteChunk(buffer, GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
