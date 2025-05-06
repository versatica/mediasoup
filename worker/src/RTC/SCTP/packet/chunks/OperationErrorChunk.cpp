#define MS_CLASS "RTC::SCTP::OperationErrorChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		OperationErrorChunk* OperationErrorChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::OPERATION_ERROR)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return OperationErrorChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		OperationErrorChunk* OperationErrorChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new OperationErrorChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::OPERATION_ERROR, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum OperationErrorChunk length.

			return chunk;
		}

		OperationErrorChunk* OperationErrorChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			auto* chunk = new OperationErrorChunk(const_cast<uint8_t*>(buffer), bufferLength);

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

		OperationErrorChunk::OperationErrorChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		OperationErrorChunk::~OperationErrorChunk()
		{
			MS_TRACE();
		}

		void OperationErrorChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::OperationErrorChunk>");
			DumpCommon(indentation);
			DumpErrorCauses(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::OperationErrorChunk>");
		}

		OperationErrorChunk* OperationErrorChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new OperationErrorChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		OperationErrorChunk* OperationErrorChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new OperationErrorChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
