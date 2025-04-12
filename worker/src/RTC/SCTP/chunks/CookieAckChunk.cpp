#define MS_CLASS "RTC::SCTP::CookieAckChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/CookieAckChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		CookieAckChunk* CookieAckChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			size_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::COOKIE_ACK)
			{
				MS_WARN_DEV("invalid chunk type");

				return nullptr;
			}

			if (chunkLength != CookieAckChunk::CookieAckChunkLength)
			{
				MS_WARN_TAG(
				  sctp, "CookieAckChunk Length field must be %zu", CookieAckChunk::CookieAckChunkLength);

				return nullptr;
			}

			auto* chunk = new CookieAckChunk(buffer, bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		CookieAckChunk* CookieAckChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < CookieAckChunk::CookieAckChunkLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new CookieAckChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::COOKIE_ACK, 0, CookieAckChunk::CookieAckChunkLength);

			// No need to invoke SetLength() since constructor invoked it with
			// CookieAckChunk fixed length.

			return chunk;
		}

		/* Instance methods. */

		CookieAckChunk::CookieAckChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(CookieAckChunk::CookieAckChunkLength);
		}

		CookieAckChunk::~CookieAckChunk()
		{
			MS_TRACE();
		}

		void CookieAckChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<CookieAckChunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("</CookieAckChunk>");
		}

		CookieAckChunk* CookieAckChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new CookieAckChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}
	} // namespace SCTP
} // namespace RTC
