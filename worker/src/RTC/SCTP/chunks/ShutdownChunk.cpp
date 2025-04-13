#define MS_CLASS "RTC::SCTP::ShutdownChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/ShutdownChunk.hpp"
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

			if (chunkLength != ShutdownChunk::ShutdownChunkLength)
			{
				MS_WARN_TAG(
				  sctp, "ShutdownChunk Length field must be %zu", ShutdownChunk::ShutdownChunkLength);

				return nullptr;
			}

			auto* chunk = new ShutdownChunk(buffer, bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		ShutdownChunk* ShutdownChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ShutdownChunk::ShutdownChunkLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new ShutdownChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::SHUTDOWN, 0, ShutdownChunk::ShutdownChunkLength);

			// Must also initialize extra fields in the header.
			chunk->SetCumulativeTsnAck(0);

			// No need to invoke SetLength() since constructor invoked it with
			// ShutdownChunk fixed length.

			return chunk;
		}

		/* Instance methods. */

		ShutdownChunk::ShutdownChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(ShutdownChunk::ShutdownChunkLength);
		}

		ShutdownChunk::~ShutdownChunk()
		{
			MS_TRACE();
		}

		void ShutdownChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ShutdownChunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("  cumulative tsn ack : %" PRIu32, GetCumulativeTsnAck());
			MS_DUMP("</ShutdownChunk>");
		}

		ShutdownChunk* ShutdownChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new ShutdownChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}

		void ShutdownChunk::SetCumulativeTsnAck(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}
	} // namespace SCTP
} // namespace RTC
