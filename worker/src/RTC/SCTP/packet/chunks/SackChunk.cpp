#define MS_CLASS "RTC::SCTP::SackChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()
#include <ranges>

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
			MS_DUMP_CLEAN(indentation, "  validated gap ack blocks:");
			for (const auto& gapAckBlock : GetValidatedGapAckBlocks())
			{
				MS_DUMP_CLEAN(
				  indentation, "  - start: %" PRIu16 ", end:%" PRIu16, gapAckBlock.start, gapAckBlock.end);
			}
			MS_DUMP_CLEAN(indentation, "  duplicate tsns:");
			for (const uint32_t duplicateTsn : GetDuplicateTsns())
			{
				MS_DUMP_CLEAN(indentation, "  - tsn: %" PRIu32, duplicateTsn);
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

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void SackChunk::SetAdvertisedReceiverWindowCredit(uint32_t value)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		std::vector<SackChunk::GapAckBlock> SackChunk::GetValidatedGapAckBlocks() const
		{
			MS_TRACE();

			const uint16_t numberOfGapAckBlocks = GetNumberOfGapAckBlocks();
			std::vector<SackChunk::GapAckBlock> gapAckBlocks;

			gapAckBlocks.reserve(numberOfGapAckBlocks);

			if (ValidateGapAckBlocks())
			{
				for (uint16_t idx{ 0 }; idx < numberOfGapAckBlocks; ++idx)
				{
					gapAckBlocks.emplace_back(GetAckBlockStartAt(idx), GetAckBlockEndAt(idx));
				}

				return gapAckBlocks;
			}

			// First: Only keep blocks that are sane.
			for (uint16_t idx{ 0 }; idx < numberOfGapAckBlocks; ++idx)
			{
				const uint16_t start = GetAckBlockStartAt(idx);
				const uint16_t end   = GetAckBlockEndAt(idx);

				if (end > start)
				{
					gapAckBlocks.emplace_back(start, end);
				}
			}

			// Not more than at most one remaining? Exit early.
			if (gapAckBlocks.size() == 1)
			{
				return gapAckBlocks;
			}

			// Sort the intervals by their start value, to aid in the merging below.
			std::ranges::sort(gapAckBlocks, {}, &SackChunk::GapAckBlock::start);

			// Merge overlapping ranges.
			size_t writeIdx{ 0 };

			for (size_t readIdx{ 1 }; readIdx < gapAckBlocks.size(); ++readIdx)
			{
				if (gapAckBlocks[writeIdx].end + 1 >= gapAckBlocks[readIdx].start)
				{
					gapAckBlocks[writeIdx].end =
					  std::max(gapAckBlocks[writeIdx].end, gapAckBlocks[readIdx].end);
				}
				else
				{
					++writeIdx;
					gapAckBlocks[writeIdx] = gapAckBlocks[readIdx];
				}
			}

			gapAckBlocks.resize(writeIdx + 1);

			return gapAckBlocks;
		}

		std::vector<uint32_t> SackChunk::GetDuplicateTsns() const
		{
			MS_TRACE();

			const uint32_t numberOfDuplicateTsns = GetNumberOfDuplicateTsns();
			std::vector<uint32_t> duplicateTsns;

			duplicateTsns.reserve(numberOfDuplicateTsns);

			for (uint32_t idx{ 0 }; idx < GetNumberOfDuplicateTsns(); ++idx)
			{
				duplicateTsns.emplace_back(GetDuplicateTsnAt(idx));
			}

			return duplicateTsns;
		}

		void SackChunk::AddAckBlock(uint16_t start, uint16_t end)
		{
			MS_TRACE();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 4);

			// Must move duplicate TSNs down.
			std::memmove(
			  GetDuplicateTsnsPointer() + 4, GetDuplicateTsnsPointer(), GetNumberOfDuplicateTsns() * 4);

			// Add the new ack block.
			Utils::Byte::Set2Bytes(GetAckBlocksPointer(), GetNumberOfGapAckBlocks() * 4, start);
			Utils::Byte::Set2Bytes(GetAckBlocksPointer(), (GetNumberOfGapAckBlocks() * 4) + 2, end);

			// Update the counter field.
			// NOTE: Do this after moving bytes.
			SetNumberOfGapAckBlocks(GetNumberOfGapAckBlocks() + 1);
		}

		void SackChunk::AddDuplicateTsn(uint32_t tsn)
		{
			MS_TRACE();

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

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void SackChunk::SetNumberOfDuplicateTsns(uint16_t value)
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 14, value);
		}

		bool SackChunk::ValidateGapAckBlocks() const
		{
			MS_TRACE();

			const uint16_t numberOfGapAckBlocks = GetNumberOfGapAckBlocks();

			if (numberOfGapAckBlocks == 0)
			{
				return true;
			}

			// Ensure that gap-ack-blocks are sorted, has an "end" that is not before
			// "start" and are non-overlapping and non-adjacent.
			uint16_t prevEnd{ 0 };

			for (uint16_t idx{ 0 }; idx < numberOfGapAckBlocks; ++idx)
			{
				const uint16_t start = GetAckBlockStartAt(idx);
				const uint16_t end   = GetAckBlockEndAt(idx);

				if (end < start)
				{
					return false;
				}
				else if (start <= (prevEnd + 1))
				{
					return false;
				}

				prevEnd = end;
			}

			return true;
		}
	} // namespace SCTP
} // namespace RTC
