#define MS_CLASS "RTC::SCTP::SackChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/SackChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		SackChunk* SackChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::SACK)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			if (chunkLength < SackChunk::SackChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "SackChunk Length field must have value greater than %zu",
				  SackChunk::SackChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new SackChunk(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		SackChunk* SackChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < SackChunk::SackChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new SackChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::SACK, 0, SackChunk::SackChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetCumulativeTsnAck(0);
			chunk->SetAdvertisedReceiverWindowCredit(0);
			chunk->SetNumberOfGapAckBlocks(0);
			chunk->SetNumberOfDuplicateTsns(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum SackChunk length.

			return chunk;
		}

		/* Instance methods. */

		SackChunk::SackChunk(const uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(SackChunk::SackChunkHeaderLength);
		}

		SackChunk::~SackChunk()
		{
			MS_TRACE();
		}

		void SackChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::SackChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  cumulative tsn ack: %" PRIu32, GetCumulativeTsnAck());
			MS_DUMP_CLEAN(
			  indentation,
			  "  advertised receiver window credit: %" PRIu32,
			  GetAdvertisedReceiverWindowCredit());
			MS_DUMP_CLEAN(indentation, "  number of gap blocks: %" PRIu16, GetNumberOfGapAckBlocks());
			MS_DUMP_CLEAN(indentation, "  number of duplicate tsns: %" PRIu16, GetNumberOfDuplicateTsns());
			MS_DUMP_CLEAN(indentation, "</SCTP::SackChunk>");
		}

		SackChunk* SackChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new SackChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}

		void SackChunk::SetCumulativeTsnAck(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void SackChunk::SetAdvertisedReceiverWindowCredit(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void SackChunk::SetNumberOfGapAckBlocks(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void SackChunk::SetNumberOfDuplicateTsns(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 14, value);
		}
	} // namespace SCTP
} // namespace RTC
