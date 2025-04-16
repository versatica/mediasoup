#define MS_CLASS "RTC::SCTP::InitChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/InitChunk.hpp"
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

			if (chunkLength < InitChunk::InitChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "InitChunk Length field must have value greater than %zu",
				  InitChunk::InitChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new InitChunk(buffer, bufferLength);

			if (!chunk->ParseParameters())
			{
				MS_WARN_DEV("failed to parse Chunk Parameters");

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

		InitChunk* InitChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < InitChunk::InitChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
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

		/* Instance methods. */

		InitChunk::InitChunk(const uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
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
			MS_DUMP_CLEAN(indentation, "</SCTP::InitChunk>");
		}

		InitChunk* InitChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new InitChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
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
	} // namespace SCTP
} // namespace RTC
