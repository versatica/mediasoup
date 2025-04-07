// #define MS_CLASS "RTC::SCTP::DataChunk"
// // #define MS_LOG_DEV_LEVEL 3

// #include "RTC/SCTP/DataChunk.hpp"
// #include "Logger.hpp"
// #include "MediaSoupErrors.hpp"
// #include <cstring> // std::memcpy()

// namespace RTC
// {
// 	namespace SCTP
// 	{
// 		/* Instance methods. */

// 		Chunk::Chunk(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
// 		{
// 			MS_TRACE();
// 		}

// 		Chunk::~Chunk()
// 		{
// 			MS_TRACE();
// 		}

// 		void Chunk::Dump() const
// 		{
// 			MS_TRACE();
// 			MS_DUMP("<Chunk>");
// 			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
// 			MS_DUMP("  type: %" PRIu8 " (%s)", GetType(), Chunk::ChunkType2String(GetType()).c_str());
// 			MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
// 			MS_DUMP(
// 			  "  length field: %" PRIu16 " (computed chunk length: %" PRIu16 ")",
// 			  GetLengthField(),
// 			  GetValueLength());
// 			MS_DUMP("</Chunk>");
// 		}

// 		Chunk* Chunk::Clone(uint8_t* buffer, size_t bufferLength) const
// 		{
// 			MS_TRACE();

// 			if (bufferLength < GetLength())
// 			{
// 				MS_THROW_TYPE_ERROR(
// 				  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
// 				  bufferLength,
// 				  GetLength());
// 			}

// 			std::memcpy(buffer, GetBuffer(), GetLength());

// 			auto* clonedChunk = new Chunk(buffer, bufferLength);

// 			// NOTE: The `frozen` flag will be false in the cloned Chunk by default.

// 			// Need to manually set Serializable length.
// 			clonedChunk->SetLength(GetLength());

// 			return clonedChunk;
// 		}

// 		void Chunk::InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t valueLength)
// 		{
// 			MS_TRACE();

// 			GetHeaderPointer()->type  = chunkType;
// 			GetHeaderPointer()->flags = flags;
// 			SetLengthField(Chunk::ChunkHeaderLength + valueLength);
// 		}
// 	} // namespace SCTP
// } // namespace RTC
