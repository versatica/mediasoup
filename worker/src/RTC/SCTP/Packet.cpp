#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/DataChunk.hpp"
#include "RTC/SCTP/ShutdownChunk.hpp"
#include "RTC/SCTP/UnknownChunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		bool Packet::IsPacket(const uint8_t* buffer, size_t bufferLength)
		{
			auto* header = reinterpret_cast<const Packet::CommonHeader*>(buffer);

			return (
			  (bufferLength >= Packet::CommonHeaderLength) &&
			  // Source and destination ports cannot be 0.
			  (uint16_t{ ntohs(header->sourcePort) } != 0 &&
			   uint16_t{ ntohs(header->destinationPort) } != 0) &&
			  // Buffer length must be multiple of 4 bytes.
			  Utils::Byte::IsPaddedTo4Bytes(bufferLength));
		}

		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!Packet::IsPacket(buffer, bufferLength))
			{
				MS_WARN_TAG(sctp, "not an SCTP Packet");

				return nullptr;
			}

			auto* packet = new Packet(buffer, bufferLength);

			// TODO: Move this to some Validate() method.
			if (packet->GetSourcePort() == 0u || packet->GetDestinationPort() == 0u)
			{
				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, SCTP Packet discarded");

				delete packet;
				return nullptr;
			}

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to other parts of the Packet.
			auto* ptr = buffer;

			// Move to chunks.
			ptr = packet->GetChunksPointer();

			while (ptr < buffer + bufferLength)
			{
				// The remaining length in the buffer is the potential buffer length
				// of the chunk.
				size_t chunkMaxBufferLength = bufferLength - (ptr - buffer);

				// Here we must anticipate the type of each chunk to use its appropriate
				// parser.
				Chunk::ChunkType chunkType;
				size_t chunkLength;
				uint8_t padding;

				if (!Chunk::IsChunk(ptr, chunkMaxBufferLength, chunkType, chunkLength, padding))
				{
					MS_WARN_TAG(sctp, "not a SCTP Chunk");

					delete packet;
					return nullptr;
				}

				Chunk* chunk{ nullptr };

				MS_DEBUG_DEV("parsing SCTP Chunk [ptr:%zu, type:%" PRIu8 "]", ptr - buffer, chunkType);

				switch (chunkType)
				{
					case Chunk::ChunkType::DATA:
					{
						chunk = DataChunk::Parse(ptr, chunkLength + padding);

						if (!chunk)
						{
							delete packet;
							return nullptr;
						}

						break;
					}

					case Chunk::ChunkType::SHUTDOWN:
					{
						chunk = ShutdownChunk::Parse(ptr, chunkLength + padding);

						if (!chunk)
						{
							delete packet;
							return nullptr;
						}

						break;
					}

					default:
					{
						chunk = UnknownChunk::Parse(ptr, chunkLength + padding);

						if (!chunk)
						{
							delete packet;
							return nullptr;
						}
					}
				}

				// Here we are parsing so we don't use AddChunk() (that clones the
				// Chunk into the Packet buffer) but AddParsedChunk().
				packet->AddParsedChunk(chunk);

				ptr += chunk->GetLength();
			}

			const size_t computedLength = ptr - buffer;

			// Ensure computed length matches the total given buffer length.
			if (computedLength != bufferLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "computed length (%zu bytes) != buffer length (%zu bytes)",
				  computedLength,
				  bufferLength);

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

			size_t computedLength = Packet::CommonHeaderLength;

			// No space for common header.
			if (bufferLength < computedLength)
			{
				MS_THROW_TYPE_ERROR("no space for common header");
			}

			auto* packet = new Packet(buffer, bufferLength);

			packet->InitializeHeader();

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		/* Instance methods. */

		Packet::Packet(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		Packet::~Packet()
		{
			MS_TRACE();

			for (auto* chunk : this->chunks)
			{
				delete chunk;
			}
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
			MS_DUMP("  has chunks: %s", HasChunks() ? "yes" : "no");
			MS_DUMP("  chunks count: %zu", GetChunksCount());
			for (auto* chunk : this->chunks)
			{
				chunk->Dump();
			}
			MS_DUMP("</Packet>");
		}

		void Packet::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			// Reassign pointers.
			for (auto* chunk : this->chunks)
			{
				size_t offset = chunk->GetBuffer() - previousBuffer;

				chunk->SetBuffer(buffer + offset);
			}
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new Packet(buffer, bufferLength);

			CloneInto(clonedPacket);

			// Add a new parsed Chunk for each Chunk in this Packet and make it
			// pointer point to its position in the new buffer.
			for (const auto* chunk : this->chunks)
			{
				size_t offset = chunk->GetBuffer() - GetBuffer();

				Chunk* clonedChunk{ nullptr };

				switch (chunk->GetType())
				{
					case Chunk::ChunkType::DATA:
					{
						clonedChunk = new DataChunk(buffer + offset, chunk->GetLength());

						break;
					}

					case Chunk::ChunkType::SHUTDOWN:
					{
						clonedChunk = new ShutdownChunk(buffer + offset, chunk->GetLength());

						break;
					}

					default:
					{
						clonedChunk = new UnknownChunk(buffer + offset, chunk->GetLength());
					}
				}

				// Set the proper Chunk length.
				clonedChunk->SetLength(chunk->GetLength());
				// Add it to the list as it if was parsed.
				clonedPacket->AddParsedChunk(clonedChunk);
			}

			return clonedPacket;
		}

		void Packet::SetSourcePort(uint16_t sourcePort)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetHeaderPointer()->sourcePort = uint16_t{ htons(sourcePort) };
		}

		void Packet::SetDestinationPort(uint16_t destinationPort)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetHeaderPointer()->destinationPort = uint16_t{ htons(destinationPort) };
		}

		void Packet::SetVerificationTag(uint32_t verificationTag)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetHeaderPointer()->verificationTag = uint32_t{ htonl(verificationTag) };
		}

		void Packet::SetChecksum(uint32_t checksum)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetHeaderPointer()->checksum = uint32_t{ htonl(checksum) };
		}

		void Packet::AddChunk(const Chunk* chunk)
		{
			MS_TRACE();

			AssertNotFrozen();

			size_t length = GetLength() + chunk->GetLength();

			// Let's append the chunk at the end of existing chunks.
			auto* clonedChunk =
			  chunk->Clone(const_cast<uint8_t*>(GetBuffer()) + GetLength(), chunk->GetLength());

			// Freeze the cloned chunk.
			clonedChunk->Freeze();

			this->chunks.push_back(clonedChunk);

			// Update Serializable length.
			SetLength(length);
		}

		Chunk* Packet::BuildChunkInPlace(Chunk::ChunkType chunkType)
		{
			MS_TRACE();

			Chunk* chunk{ nullptr };

			// The new Chunk will be added after other Chunks in the Packet, this is,
			// at the end of the Packet.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
			// The remaining length in the buffer is the potential buffer length
			// of the chunk.
			size_t chunkMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

			switch (chunkType)
			{
				case Chunk::ChunkType::DATA:
				{
					chunk = DataChunk::Factory(ptr, chunkMaxBufferLength);

					break;
				}

				case Chunk::ChunkType::SHUTDOWN:
				{
					chunk = ShutdownChunk::Factory(ptr, chunkMaxBufferLength);

					break;
				}
			}

			// NOTE: Do not fix/update the Chunk buffer length since the caller
			// probably wants to modify the Chunk.

			// When the application completes the Chunk it must call
			// `chunk->Consolidate()` and that will trigger this event.
			chunk->SetConsolidatedListener(
			  [this, chunk]()
			  {
				  // Fix buffer length assigned to the Chunk.
				  chunk->SetBufferLength(chunk->GetLength());

				  // NOTE: No need to freeze the Chunk because `Consolidate()` did it.

				  // Add the Chunk to the list.
				  this->chunks.push_back(chunk);

				  // Update Packet length.
				  SetLength(GetLength() + chunk->GetLength());
			  });

			return chunk;
		}

		void Packet::InitializeHeader()
		{
			MS_TRACE();

			SetSourcePort(0u);
			SetDestinationPort(0u);
			SetVerificationTag(0u);
			SetChecksum(0u);
		}

		void Packet::AddParsedChunk(Chunk* chunk)
		{
			MS_TRACE();

			// Freeze the chunk.
			chunk->Freeze();

			this->chunks.push_back(chunk);
		}
	} // namespace SCTP
} // namespace RTC
