#define MS_CLASS "RTC::SCTP::CookieEchoChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/CookieEchoChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()

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

			auto* chunk = new CookieEchoChunk(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		CookieEchoChunk* CookieEchoChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < CookieEchoChunk::CookieEchoChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new CookieEchoChunk(buffer, bufferLength);

			chunk->InitializeHeader(
			  Chunk::ChunkType::COOKIE_ECHO, 0, CookieEchoChunk::CookieEchoChunkHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum CookieEchoChunk length.

			return chunk;
		}

		/* Instance methods. */

		CookieEchoChunk::CookieEchoChunk(const uint8_t* buffer, size_t bufferLength)
		  : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(CookieEchoChunk::CookieEchoChunkHeaderLength);
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

			auto* clonedItem = new CookieEchoChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
		}

		void CookieEchoChunk::SetCookie(const uint8_t* cookie, uint16_t cookieLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength       = GetLength();
			auto previousLengthField  = GetLengthField();
			auto previousCookieLength = GetCookieLength();
			auto newNotPaddedLength =
			  size_t{ previousLengthField } - size_t{ previousCookieLength } + size_t{ cookieLength };
			auto newPaddedLength = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			try
			{
				// Let's call SetLength() on parent with the new computed Chunk length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				// NOTE: Chunks must be padded to 4 bytes.
				SetLength(newPaddedLength);

				// Update the Chunk Length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newNotPaddedLength);
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}

			// Copy the given cookie into the buffer.
			std::memmove(GetValuePointer(), cookie, cookieLength);

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}
	} // namespace SCTP
} // namespace RTC
