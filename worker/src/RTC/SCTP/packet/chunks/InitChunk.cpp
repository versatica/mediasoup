#define MS_CLASS "RTC::SCTP::InitChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		InitChunk* InitChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::INIT)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return InitChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		InitChunk* InitChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < InitChunk::InitChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new InitChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::INIT, 0, InitChunk::InitChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetInitiateTag(0);
			chunk->SetAdvertisedReceiverWindowCredit(0);
			chunk->SetNumberOfOutboundStreams(0);
			chunk->SetNumberOfInboundStreams(0);
			chunk->SetInitialTsn(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum InitChunk length.

			return chunk;
		}

		InitChunk* InitChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < InitChunk::InitChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "InitChunk Length field must be equal or greater than %zu",
				  InitChunk::InitChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new InitChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Parse Parameters.
			if (!chunk->ParseParameters())
			{
				MS_WARN_DEV("failed to parse Parameters");

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

		InitChunk::InitChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(InitChunk::InitChunkHeaderLength);
		}

		InitChunk::~InitChunk()
		{
			MS_TRACE();
		}

		void InitChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::InitChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  initiate tag: %" PRIu32, GetInitiateTag());
			MS_DUMP_CLEAN(
			  indentation,
			  "  advertised receiver window credit: %" PRIu32,
			  GetAdvertisedReceiverWindowCredit());
			MS_DUMP_CLEAN(
			  indentation, "  number of outbound streams: %" PRIu16, GetNumberOfOutboundStreams());
			MS_DUMP_CLEAN(indentation, "  number of inbound streams: %" PRIu16, GetNumberOfInboundStreams());
			MS_DUMP_CLEAN(indentation, "  initial tsn: %" PRIu32, GetInitialTsn());
			DumpParameters(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::InitChunk>");
		}

		InitChunk* InitChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new InitChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void InitChunk::SetInitiateTag(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void InitChunk::SetAdvertisedReceiverWindowCredit(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void InitChunk::SetNumberOfOutboundStreams(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void InitChunk::SetNumberOfInboundStreams(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 14, value);
		}

		void InitChunk::SetInitialTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, value);
		}

		InitChunk* InitChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new InitChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
