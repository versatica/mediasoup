#define MS_CLASS "RTC::SCTP::CookieEchoChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		CookieEchoChunk* CookieEchoChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::COOKIE_ECHO)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return CookieEchoChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		CookieEchoChunk* CookieEchoChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new CookieEchoChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::COOKIE_ECHO, 0, Chunk::ChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum CookieEchoChunk length.

			return chunk;
		}

		CookieEchoChunk* CookieEchoChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			auto* chunk = new CookieEchoChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		CookieEchoChunk::CookieEchoChunk(uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Chunk::ChunkHeaderLength);
		}

		CookieEchoChunk::~CookieEchoChunk()
		{
			MS_TRACE();
		}

		void CookieEchoChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::CookieEchoChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  cookie length: %" PRIu16 " (has cookie: %s)",
			  GetCookieLength(),
			  HasCookie() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::CookieEchoChunk>");
		}

		CookieEchoChunk* CookieEchoChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new CookieEchoChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void CookieEchoChunk::SetCookie(const uint8_t* cookie, uint16_t cookieLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(cookie, cookieLength);
		}

		CookieEchoChunk* CookieEchoChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new CookieEchoChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
