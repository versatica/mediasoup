#define MS_CLASS "RTC::SCTP::UnknownChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
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

			return UnknownChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		UnknownChunk* UnknownChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			auto* chunk = new UnknownChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		UnknownChunk::UnknownChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		UnknownChunk::~UnknownChunk()
		{
			MS_TRACE();
		}

		void UnknownChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::UnknownChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unknown value length: %" PRIu16 " (has unknown value: %s)",
			  GetUnknownValueLength(),
			  HasUnknownValue() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::UnknownChunk>");
		}

		UnknownChunk* UnknownChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new UnknownChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		UnknownChunk* UnknownChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new UnknownChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
