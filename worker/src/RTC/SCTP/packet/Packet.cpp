#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if ((bufferLength < Packet::CommonHeaderLength) || !Utils::Byte::IsPaddedTo4Bytes(bufferLength))
			{
				MS_WARN_TAG(sctp, "not an SCTP Packet");

				return nullptr;
			}

			auto* packet = new Packet(const_cast<uint8_t*>(buffer), bufferLength);

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to other parts of the Packet.
			auto* ptr = buffer;

			// Move to chunks.
			ptr = packet->GetChunksPointer();

			while (ptr < buffer + bufferLength)
			{
				// The remaining length in the buffer is the potential buffer length
				// of the Chunk.
				size_t chunkMaxBufferLength = bufferLength - (ptr - buffer);

				// Here we must anticipate the type of each Chunk to use its appropriate
				// parser.
				Chunk::ChunkType chunkType;
				uint16_t chunkLength;
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
						chunk = DataChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::INIT:
					{
						chunk = InitChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::INIT_ACK:
					{
						chunk = InitAckChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::SACK:
					{
						chunk = SackChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::HEARTBEAT_REQUEST:
					{
						chunk =
						  HeartbeatRequestChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::HEARTBEAT_ACK:
					{
						chunk = HeartbeatAckChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::ABORT:
					{
						chunk =
						  AbortAssociationChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::SHUTDOWN:
					{
						chunk = ShutdownChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::SHUTDOWN_ACK:
					{
						chunk = ShutdownAckChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::OPERATION_ERROR:
					{
						chunk =
						  OperationErrorChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::COOKIE_ECHO:
					{
						chunk = CookieEchoChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::COOKIE_ACK:
					{
						chunk = CookieAckChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::SHUTDOWN_COMPLETE:
					{
						chunk =
						  ShutdownCompleteChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::FORWARD_TSN:
					{
						chunk = ForwardTsnChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::RE_CONFIG:
					{
						chunk = ReConfigChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::I_DATA:
					{
						chunk = IDataChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					case Chunk::ChunkType::I_FORWARD_TSN:
					{
						chunk = IForwardTsnChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);

						break;
					}

					default:
					{
						chunk = UnknownChunk::ParseStrict(ptr, chunkLength + padding, chunkLength, padding);
					}
				}

				if (!chunk)
				{
					delete packet;
					return nullptr;
				}

				packet->chunks.push_back(chunk);

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

			// Must initialize extra fields in the header.
			packet->SetSourcePort(0u);
			packet->SetDestinationPort(0u);
			packet->SetVerificationTag(0u);
			packet->SetChecksum(0u);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		/* Instance methods. */

		Packet::Packet(uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		Packet::~Packet()
		{
			MS_TRACE();

			for (const auto* chunk : this->chunks)
			{
				delete chunk;
			}
		}

		void Packet::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::Packet>");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  frozen: %s", IsFrozen() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  source port: %" PRIu16, GetSourcePort());
			MS_DUMP_CLEAN(indentation, "  destination port: %" PRIu16, GetDestinationPort());
			MS_DUMP_CLEAN(indentation, "  verification tag: %" PRIu32, GetVerificationTag());
			MS_DUMP_CLEAN(indentation, "  checksum: %" PRIu32, GetChecksum());
			MS_DUMP_CLEAN(indentation, "  chunks count: %zu", GetChunksCount());
			for (const auto* chunk : this->chunks)
			{
				chunk->Dump(indentation + 1);
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::Packet>");
		}

		void Packet::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			for (auto* chunk : this->chunks)
			{
				size_t offset = chunk->GetBuffer() - previousBuffer;

				chunk->SoftSerialize(buffer + offset);
			}
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new Packet(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// Soft clone Packet Chunks into the given cloned Packet.
			for (auto* chunk : this->chunks)
			{
				size_t offset = chunk->GetBuffer() - GetBuffer();

				auto* softClonedChunk = chunk->SoftClone(buffer + offset);

				// Chunk constructors don't freeze the Chunk so we must do it manually.
				softClonedChunk->Freeze();

				clonedPacket->chunks.push_back(softClonedChunk);
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

			// Let's append the Chunk at the end of existing Chunks.
			auto* clonedChunk =
			  chunk->Clone(const_cast<uint8_t*>(GetBuffer()) + GetLength(), chunk->GetLength());

			// Update Serializable length.
			try
			{
				SetLength(length);
			}
			catch (const MediaSoupError& error)
			{
				delete clonedChunk;

				throw;
			}

			// Freeze the cloned Chunk.
			clonedChunk->Freeze();

			this->chunks.push_back(clonedChunk);
		}

		void Packet::SetCRC32cChecksum()
		{
			MS_TRACE();

			AssertNotFrozen();

			SetChecksum(0u);

			auto crc32c = Utils::Crypto::GetCRC32c(GetBuffer(), GetLength());

			SetChecksum(crc32c);
		}

		bool Packet::ValidateCRC32cChecksum() const
		{
			MS_TRACE();

			auto crc32c = GetChecksum();

			// NOTE: Cannot use SetChecksum() because it throws if Packet is frozen.
			GetHeaderPointer()->checksum = 0;

			auto computedCrc32c = Utils::Crypto::GetCRC32c(GetBuffer(), GetLength());

			GetHeaderPointer()->checksum = uint32_t{ htonl(crc32c) };

			return computedCrc32c == crc32c;
		}

		void Packet::HandleInPlaceChunk(Chunk* chunk)
		{
			MS_TRACE();

			// When the application completes the Chunk it must call
			// `chunk->Consolidate()` and that will trigger this event.
			chunk->SetConsolidatedListener(
			  [this, chunk]()
			  {
				  // Fix buffer length assigned to the Chunk.
				  chunk->SetBufferLength(chunk->GetLength());

				  // Freeze the Chunk.
				  chunk->Freeze();

				  // Update Packet length.
				  // NOTE: This will throw if there is no enough space in the Packet
				  // buffer.
				  SetLength(GetLength() + chunk->GetLength());

				  // Add the Chunk to the list.
				  this->chunks.push_back(chunk);
			  });
		}
	} // namespace SCTP
} // namespace RTC
