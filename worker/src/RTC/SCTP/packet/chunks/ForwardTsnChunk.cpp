#define MS_CLASS "RTC::SCTP::ForwardTsnChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		ForwardTsnChunk* ForwardTsnChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::FORWARD_TSN)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return ForwardTsnChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		ForwardTsnChunk* ForwardTsnChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ForwardTsnChunk::ForwardTsnChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new ForwardTsnChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::FORWARD_TSN, 0, ForwardTsnChunk::ForwardTsnChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetNewCumulativeTsn(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum ForwardTsnChunk length.

			return chunk;
		}

		ForwardTsnChunk* ForwardTsnChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < ForwardTsnChunk::ForwardTsnChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "ForwardTsnChunk Length field must be equal or greater than %zu",
				  ForwardTsnChunk::ForwardTsnChunkHeaderLength);

				return nullptr;
			}

			// Here we must validate that length is multiple of 4.
			if (chunkLength % 4 != 0)
			{
				MS_WARN_TAG(sctp, "wrong length (not multiple of 4)");

				return nullptr;
			}

			auto* chunk = new ForwardTsnChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		ForwardTsnChunk::ForwardTsnChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ForwardTsnChunk::ForwardTsnChunkHeaderLength);
		}

		ForwardTsnChunk::~ForwardTsnChunk()
		{
			MS_TRACE();
		}

		void ForwardTsnChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ForwardTsnChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  new cumulative tsn: %" PRIu32, GetNewCumulativeTsn());
			MS_DUMP_CLEAN(indentation, "  number of streams: %" PRIu16, GetNumberOfStreams());
			for (uint16_t idx{ 0 }; idx < GetNumberOfStreams(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - idx: %" PRIu16 ", stream: %" PRIu16 ", stream sequence:%" PRIu16,
				  idx,
				  GetStreamAt(idx),
				  GetStreamSequenceAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::ForwardTsnChunk>");
		}

		ForwardTsnChunk* ForwardTsnChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new ForwardTsnChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void ForwardTsnChunk::SetNewCumulativeTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void ForwardTsnChunk::AddStream(uint16_t stream, uint16_t streamSequence)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousVariableLengthValueLength = GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(previousVariableLengthValueLength + 4);

			// Add the new stream and stream sequence.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength, stream);
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength + 2, streamSequence);
		}

		ForwardTsnChunk* ForwardTsnChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new ForwardTsnChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
