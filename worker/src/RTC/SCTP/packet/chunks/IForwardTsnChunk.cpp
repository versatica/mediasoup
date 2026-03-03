#define MS_CLASS "RTC::SCTP::IForwardTsnChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IForwardTsnChunk* IForwardTsnChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::I_FORWARD_TSN)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return IForwardTsnChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		IForwardTsnChunk* IForwardTsnChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IForwardTsnChunk::IForwardTsnChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new IForwardTsnChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::I_FORWARD_TSN, 0, IForwardTsnChunk::IForwardTsnChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetNewCumulativeTsn(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum IForwardTsnChunk length.

			return chunk;
		}

		IForwardTsnChunk* IForwardTsnChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < IForwardTsnChunk::IForwardTsnChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IForwardTsnChunk Length field must be equal or greater than %zu",
				  IForwardTsnChunk::IForwardTsnChunkHeaderLength);

				return nullptr;
			}

			// Here we must validate that length is multiple of 8.
			if (chunkLength % 8 != 0)
			{
				MS_WARN_TAG(sctp, "wrong length (not multiple of 4)");

				return nullptr;
			}

			auto* chunk = new IForwardTsnChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			return chunk;
		}

		/* Instance methods. */

		IForwardTsnChunk::IForwardTsnChunk(uint8_t* buffer, size_t bufferLength)
		  : AnyForwardTsnChunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IForwardTsnChunk::IForwardTsnChunkHeaderLength);
		}

		IForwardTsnChunk::~IForwardTsnChunk()
		{
			MS_TRACE();
		}

		void IForwardTsnChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::IForwardTsnChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  new cumulative tsn: %" PRIu32, GetNewCumulativeTsn());
			MS_DUMP_CLEAN(indentation, "  number of skipped streams: %" PRIu16, GetNumberOfSkippedStreams());
			MS_DUMP_CLEAN(indentation, "  skipped streams:");
			for (auto& skippedStream : GetSkippedStreams())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - stream id: %" PRIu16 ", unordered:%s, mid:%" PRIu32,
				  skippedStream.streamId,
				  skippedStream.unordered ? "yes" : "no",
				  skippedStream.mid);
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::IForwardTsnChunk>");
		}

		IForwardTsnChunk* IForwardTsnChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new IForwardTsnChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void IForwardTsnChunk::SetNewCumulativeTsn(uint32_t value)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		std::vector<AnyForwardTsnChunk::SkippedStream> IForwardTsnChunk::GetSkippedStreams() const
		{
			MS_TRACE();

			std::vector<AnyForwardTsnChunk::SkippedStream> skippedStreams;
			const uint16_t numSkippedStreams = GetNumberOfSkippedStreams();

			skippedStreams.reserve(numSkippedStreams);

			for (uint16_t idx{ 0 }; idx < numSkippedStreams; ++idx)
			{
				skippedStreams.emplace_back(GetStreamIdAt(idx), GetUFlagAt(idx), GetMessageIdentifierAt(idx));
			}

			return skippedStreams;
		}

		void IForwardTsnChunk::AddStream(uint16_t stream, bool uFlag, uint32_t messageIdentifier)
		{
			MS_TRACE();

			auto previousVariableLengthValueLength = GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(previousVariableLengthValueLength + 8);

			// Add the new stream, flag U and message identifier.
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength, stream);
			Utils::Byte::Set2Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength + 2, uFlag);
			Utils::Byte::Set4Bytes(
			  GetVariableLengthValuePointer(), previousVariableLengthValueLength + 4, messageIdentifier);
		}

		IForwardTsnChunk* IForwardTsnChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new IForwardTsnChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
