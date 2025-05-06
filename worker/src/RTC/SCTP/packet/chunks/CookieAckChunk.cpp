#define MS_CLASS "RTC::SCTP::CookieAckChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
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
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::COOKIE_ACK)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return CookieAckChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		CookieAckChunk* CookieAckChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new CookieAckChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::COOKIE_ACK, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// CookieAckChunk fixed length.

			return chunk;
		}

		CookieAckChunk* CookieAckChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength != Chunk::ChunkHeaderLength)
			{
				MS_WARN_TAG(sctp, "CookieAckChunk Length field must be %zu", Chunk::ChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new CookieAckChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		CookieAckChunk::CookieAckChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLength(Chunk::ChunkHeaderLength);
		}

		CookieAckChunk::~CookieAckChunk()
		{
			MS_TRACE();
		}

		void CookieAckChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::CookieAckChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::CookieAckChunk>");
		}

		CookieAckChunk* CookieAckChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new CookieAckChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		CookieAckChunk* CookieAckChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new CookieAckChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
