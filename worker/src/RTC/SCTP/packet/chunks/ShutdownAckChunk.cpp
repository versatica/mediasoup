#define MS_CLASS "RTC::SCTP::ShutdownAckChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
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

			return ShutdownAckChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		ShutdownAckChunk* ShutdownAckChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new ShutdownAckChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::SHUTDOWN_ACK, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// ShutdownAckChunk fixed length.

			return chunk;
		}

		ShutdownAckChunk* ShutdownAckChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength != Chunk::ChunkHeaderLength)
			{
				MS_WARN_TAG(sctp, "ShutdownAckChunk Length field must be %zu", Chunk::ChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new ShutdownAckChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		ShutdownAckChunk::ShutdownAckChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(Chunk::ChunkHeaderLength);
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

			auto* clonedChunk = new ShutdownAckChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		ShutdownAckChunk* ShutdownAckChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new ShutdownAckChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
