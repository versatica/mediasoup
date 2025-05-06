#define MS_CLASS "RTC::SCTP::ShutdownChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ShutdownChunk* ShutdownChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::SHUTDOWN)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return ShutdownChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		ShutdownChunk* ShutdownChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ShutdownChunk::ShutdownChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new ShutdownChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::SHUTDOWN, 0, ShutdownChunk::ShutdownChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetCumulativeTsnAck(0);

			// No need to invoke SetLength() since constructor invoked it with
			// ShutdownChunk fixed length.

			return chunk;
		}

		ShutdownChunk* ShutdownChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength != ShutdownChunk::ShutdownChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp, "ShutdownChunk Length field must be %zu", ShutdownChunk::ShutdownChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new ShutdownChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		ShutdownChunk::ShutdownChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(ShutdownChunk::ShutdownChunkHeaderLength);
		}

		ShutdownChunk::~ShutdownChunk()
		{
			MS_TRACE();
		}

		void ShutdownChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ShutdownChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  cumulative tsn ack : %" PRIu32, GetCumulativeTsnAck());
			MS_DUMP_CLEAN(indentation, "</SCTP::ShutdownChunk>");
		}

		ShutdownChunk* ShutdownChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new ShutdownChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void ShutdownChunk::SetCumulativeTsnAck(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		ShutdownChunk* ShutdownChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new ShutdownChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
