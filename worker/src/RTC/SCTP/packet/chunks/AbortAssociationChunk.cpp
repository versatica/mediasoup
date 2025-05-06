#define MS_CLASS "RTC::SCTP::AbortAssociationChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		AbortAssociationChunk* AbortAssociationChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::ABORT)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return AbortAssociationChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		AbortAssociationChunk* AbortAssociationChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new AbortAssociationChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::ABORT, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum AbortAssociationChunk length.

			return chunk;
		}

		AbortAssociationChunk* AbortAssociationChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			auto* chunk = new AbortAssociationChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Parse Error Causes.
			if (!chunk->ParseErrorCauses())
			{
				MS_WARN_DEV("failed to parse Error Causes");

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

		AbortAssociationChunk::AbortAssociationChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		AbortAssociationChunk::~AbortAssociationChunk()
		{
			MS_TRACE();
		}

		void AbortAssociationChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::AbortAssociationChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  flag T: %" PRIu8, GetT());
			DumpErrorCauses(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::AbortAssociationChunk>");
		}

		AbortAssociationChunk* AbortAssociationChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new AbortAssociationChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void AbortAssociationChunk::SetT(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit0(flag);
		}

		AbortAssociationChunk* AbortAssociationChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new AbortAssociationChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
