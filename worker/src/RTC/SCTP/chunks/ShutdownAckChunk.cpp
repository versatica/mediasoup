#define MS_CLASS "RTC::SCTP::ShutdownAckChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/ShutdownAckChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ShutdownAckChunk* ShutdownAckChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::SHUTDOWN_ACK)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			if (chunkLength != ShutdownAckChunk::ShutdownAckChunkLength)
			{
				MS_WARN_TAG(
				  sctp, "ShutdownAckChunk Length field must be %zu", ShutdownAckChunk::ShutdownAckChunkLength);

				return nullptr;
			}

			auto* chunk = new ShutdownAckChunk(buffer, bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		ShutdownAckChunk* ShutdownAckChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ShutdownAckChunk::ShutdownAckChunkLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new ShutdownAckChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::SHUTDOWN_ACK, 0, ShutdownAckChunk::ShutdownAckChunkLength);

			// No need to invoke SetLength() since constructor invoked it with
			// ShutdownAckChunk fixed length.

			return chunk;
		}

		/* Instance methods. */

		ShutdownAckChunk::ShutdownAckChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(ShutdownAckChunk::ShutdownAckChunkLength);
		}

		ShutdownAckChunk::~ShutdownAckChunk()
		{
			MS_TRACE();
		}

		void ShutdownAckChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ShutdownAckChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::ShutdownAckChunk>");
		}

		ShutdownAckChunk* ShutdownAckChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new ShutdownAckChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}
	} // namespace SCTP
} // namespace RTC
