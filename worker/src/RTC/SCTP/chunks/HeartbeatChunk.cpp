#define MS_CLASS "RTC::SCTP::HeartbeatChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/HeartbeatChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		HeartbeatChunk* HeartbeatChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			size_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::HEARTBEAT)
			{
				MS_WARN_DEV("invalid chunk type");

				return nullptr;
			}

			auto* chunk = new HeartbeatChunk(buffer, bufferLength);

			// TODO: Parse Parameters.

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		HeartbeatChunk* HeartbeatChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < HeartbeatChunk::HeartbeatChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new HeartbeatChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::HEARTBEAT, 0, HeartbeatChunk::HeartbeatChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum HeartbeatChunk length.

			return chunk;
		}

		/* Instance methods. */

		HeartbeatChunk::HeartbeatChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(HeartbeatChunk::HeartbeatChunkHeaderLength);
		}

		HeartbeatChunk::~HeartbeatChunk()
		{
			MS_TRACE();
		}

		void HeartbeatChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<HeartbeatChunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			// TODO: Parameters.
			MS_DUMP("</HeartbeatChunk>");
		}

		HeartbeatChunk* HeartbeatChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new HeartbeatChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			// TODO: Clone Parameters.

			return clonedItem;
		}
	} // namespace SCTP
} // namespace RTC
