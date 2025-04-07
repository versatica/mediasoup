#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		bool Packet::IsSctp(const uint8_t* buffer, size_t bufferLength)
		{
			auto* header = reinterpret_cast<const Packet::CommonHeader*>(buffer);

			return (
			  (bufferLength >= Packet::CommonHeaderLength) &&
			  // Source and destination ports cannot be 0.
			  (uint16_t{ ntohs(header->sourcePort) } != 0 &&
			   uint16_t{ ntohs(header->destinationPort) } != 0) &&
			  // Length must be multiple of 4 bytes.
			  Utils::Byte::IsPaddedTo4Bytes(bufferLength));
		}

		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!Packet::IsSctp(buffer, bufferLength))
			{
				MS_WARN_TAG(sctp, "not an SCTP packet");

				return nullptr;
			}

			auto* packet = new Packet(buffer, bufferLength);

			// TODO: Move this to some Validate() method.
			if (packet->GetSourcePort() == 0u || packet->GetDestinationPort() == 0u)
			{
				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, packet discarded");

				delete packet;
				return nullptr;
			}

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to other parts of the Packet.
			auto* ptr = buffer;

			// Move to chunks.
			ptr = packet->GetChunksPointer();

			// while (ptr < buffer + bufferLength)
			// {
			// 	// The remaining length in the buffer is the potential buffer length
			// 	// of the chunk.
			// 	size_t chunkBufferLength = packet->GetEndPointer() - ptr;

			//  // Here we must anticipate the type of each chunk to use its appropriate
			// 	// parser.
			// 	Chunk::ChunkType chunkType;
			// 	uint16_t chunkLength;

			// 	if (!Chunk::IsChunk(ptr, itemBufferLength, chunkType, chunkLength))
			// 	{
			// 		MS_WARN_DEV("not a Chunk");

			// 		delete packet;
			// 		return nullptr;
			// 	}

			// 	Chunk* chunk{ nullptr };

			// 	MS_DEBUG_DEV("parsing Chunk [ptr:%zu, type:%" PRIu8 "]", ptr - buffer, chunkType);

			// 	switch (chunkType)
			// 	{
			// 		case Chunk::ChunkType::XXXXX:
			// 		{
			// 			chunk = XxxxxChunk::Parse(ptr, itemBufferLength);

			// 			if (!chunk)
			// 			{
			// 				MS_WARN_DEV("XxxxxChunk parser failed");

			// 				delete packet;
			// 				return nullptr;
			// 			}

			// 			break;
			// 		}

			// 		default:
			// 		{
			// 			chunk = UnknownChunk::Parse(ptr, itemBufferLength);

			// 			if (!chunk)
			// 			{
			// 				MS_WARN_DEV("UnknownChunk parser failed");

			// 				delete packet;
			// 				return nullptr;
			// 			}
			// 		}
			// 	}

			// 	// Let's fix chunk's buffer length. This is because we didn't know its
			// 	// exact length when we called FooItem::Parse() so we passed the rest
			// 	// of the Packet buffer as buffer length. Once chunk is parsed, and
			// 	// given that it is part of the Packet buffer, we can fix its buffer
			// 	// length by making it be equal to its real length.
			// 	chunk->SetBufferLength(chunk->GetLength());

			// 	// Here we are parsing so we don't use AddChunk() (that clones the
			// 	// Chunk into the Packet buffer) but AddParsedChunk().
			// 	packet->AddParsedChunk(chunk);

			// 	ptr += chunk->GetLength();
			// }

			const size_t computedLength = ptr - buffer;

			// Ensure computed length matches the total given buffer length.
			if (computedLength != bufferLength)
			{
				MS_WARN_DEV("computed padded length != buffer length");

				delete packet;
				return nullptr;
			}

			// It's mandatory to call SetLength() once we are done and we know the
			// exact length of the Packet.
			packet->SetLength(computedLength);

			// Mark the packet as frozen since we are parsing.
			packet->Freeze();

			return packet;
		}

		Packet* Packet::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			// TODO

			return nullptr;
		}

		/* Instance methods. */

		Packet::Packet(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Packet::~Packet()
		{
			MS_TRACE();

			// TODO
			// for (auto* chunk : this->chunks)
			// {
			// 	delete chunk;
			// }
		}

		void Packet::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<Packet>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP("  source port: %" PRIu16, GetSourcePort());
			MS_DUMP("  destination port: %" PRIu16, GetDestinationPort());
			MS_DUMP("  verification tag: %" PRIu32, GetVerificationTag());
			MS_DUMP("  checksum: %" PRIu32, GetChecksum());
			// TODO
			// MS_DUMP("  has chunks: %s", HasChunks() ? "yes" : "no");
			// MS_DUMP("  chunks count: %zu", GetChunksCount());
			// for (auto* chunk : this->chunks)
			// {
			// 	chunk->Dump();
			// }
			MS_DUMP("</Packet>");
		}

		void Packet::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < GetLength())
			{
				MS_THROW_TYPE_ERROR(
				  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
				  bufferLength,
				  GetLength());
			}

			size_t chunksOffset = GetChunksPointer() - GetBuffer();

			// Copy all bytes from beginning of the buffer until the position of the
			// chunks.
			std::memcpy(buffer, GetBuffer(), chunksOffset);

			// Serialize each chunk into the new buffer.
			auto* ptr = buffer + chunksOffset;

			// TODO
			// for (auto* chunk : this->chunks)
			// {
			// 	chunk->Serialize(ptr, chunk->GetLength());

			// 	// After calling `Serialize()` on the chunk, its `frozen` flag is
			// 	// reverted to false, but we want it to remain set because it's a
			// 	// chunk within the packet.
			// 	chunk->Freeze();

			// 	ptr += chunk->GetLength();
			// }

			// Manually update buffer and buffer length.
			SetBuffer(buffer);
			SetBufferLength(bufferLength);

			// May unfreeze the packet (but not its items).
			Unfreeze();
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			// TODO

			return nullptr;
		}
	} // namespace SCTP
} // namespace RTC
