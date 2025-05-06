#define MS_CLASS "RTC::SCTP::SackChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()

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

			return SackChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		SackChunk* SackChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < SackChunk::SackChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
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

		SackChunk* SackChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < SackChunk::SackChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "SackChunk Length field must be equal or greater than %zu",
				  SackChunk::SackChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new SackChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// In this Chunk we must validate that some fields have correct values.
			if (
			  (chunk->GetNumberOfGapAckBlocks() * 4) + (chunk->GetNumberOfDuplicateTsns() * 4) !=
			  chunkLength - SackChunk::SackChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp, "wrong values in Number of Gap Ack Blocks and/or Number of Duplicate TSNs fields");

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

		SackChunk::SackChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
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
			MS_DUMP_CLEAN(indentation, "  gap blocks: %" PRIu16, GetNumberOfGapAckBlocks());
			for (uint16_t idx{ 0 }; idx < GetNumberOfGapAckBlocks(); ++idx)
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - idx: %" PRIu16 ", start: %" PRIu16 ", end:%" PRIu16,
				  idx,
				  GetAckBlockStartAt(idx),
				  GetAckBlockEndAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "  duplicate tsns: %" PRIu16, GetNumberOfDuplicateTsns());
			for (uint16_t idx{ 0 }; idx < GetNumberOfDuplicateTsns(); ++idx)
			{
				MS_DUMP_CLEAN(indentation, "  - idx: %" PRIu16 ", tsn: %" PRIu32, idx, GetDuplicateTsnAt(idx));
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::SackChunk>");
		}

		SackChunk* SackChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new SackChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
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

		void SackChunk::AddAckBlock(uint16_t start, uint16_t end)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 4);

			// Must move duplicate TSNs down.
			std::memmove(
			  GetDuplicateTsnsPointer() + 4, GetDuplicateTsnsPointer(), GetNumberOfDuplicateTsns() * 4);

			// Add the new ack block.
			Utils::Byte::Set2Bytes(GetAckBlocksPointer(), GetNumberOfGapAckBlocks() * 4, start);
			Utils::Byte::Set2Bytes(GetAckBlocksPointer(), GetNumberOfGapAckBlocks() * 4 + 2, end);

			// Update the counter field.
			// NOTE: Do this after moving bytes.
			SetNumberOfGapAckBlocks(GetNumberOfGapAckBlocks() + 1);
		}

		void SackChunk::AddDuplicateTsn(uint32_t tsn)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 4);

			// Add the new duplicate TSN.
			Utils::Byte::Set4Bytes(GetDuplicateTsnsPointer(), GetNumberOfDuplicateTsns() * 4, tsn);

			// Update the counter field.
			// NOTE: Do this after moving bytes.
			SetNumberOfDuplicateTsns(GetNumberOfDuplicateTsns() + 1);
		}

		SackChunk* SackChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new SackChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
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
