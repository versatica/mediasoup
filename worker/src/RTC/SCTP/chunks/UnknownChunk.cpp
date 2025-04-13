#define MS_CLASS "RTC::SCTP::UnknownChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/UnknownChunk.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		UnknownChunk* UnknownChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			auto* chunk = new UnknownChunk(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		UnknownChunk::UnknownChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		UnknownChunk::~UnknownChunk()
		{
			MS_TRACE();
		}

		void UnknownChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<UnknownChunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
			MS_DUMP(
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")", GetLengthField(), GetValueLength());
			MS_DUMP("</UnknownChunk>");
		}

		UnknownChunk* UnknownChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new UnknownChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}
	} // namespace SCTP
} // namespace RTC
