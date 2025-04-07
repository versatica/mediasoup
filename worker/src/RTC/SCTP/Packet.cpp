// #define MS_CLASS "RTC::SCTP::Packet"
// // #define MS_LOG_DEV_LEVEL 3

// #include "RTC/SCTP/Packet.hpp"
// #include "Logger.hpp"
// #include "Utils.hpp"
// #include <cstring> // std::memcpy()

// namespace RTC
// {
// 	namespace SCTP
// 	{
// 		/* Class methods. */

// 		bool Packet::IsSctp(const uint8_t* data, size_t len)
// 		{
// 			auto* header =
// 			  const_cast<Packet::CommonHeader*>(reinterpret_cast<const Packet::CommonHeader*>(data));

// 			// clang-format off
// 			return (
// 				(len >= Packet::CommonHeaderSize) &&
// 				// Source and destination ports cannot be 0.
// 				(header->sourcePort != 0 && header->destinationPort != 0) &&
// 				// Size must be multiple of 4 bytes.
// 				Utils::Byte::IsPaddedTo4Bytes(len)
// 			);
// 		}

// 		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
// 		{
// 			MS_TRACE();

// 			if (!Packet::IsSctp(buffer, length))
// 			{
// 				MS_WARN_TAG(sctp, "not an SCTP packet");

// 				return nullptr;
// 			}

// 			// NOTE: Here we are passing `length` as `bufferLength`. However we know
// 			// that, due to FooPacket nature, an SCTP Packet must occupy the whole
// 			// given buffer.
// 			auto* packet = new Packet(buffer, length);

// 			// TODO: Move this to some Validate() method.
// 			if (packet->GetSourcePort() == 0u || packet->GetDestinationPort() == 0u)
// 			{
// 				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, packet discarded");

// 				delete packet;
// 				return nullptr;
// 			}

// 			// Pointer that initially points to the given data buffer and is later
// 			// incremented to point to other parts of the Packet.
// 			auto* ptr = buffer;

// 			// Move to chunks.
// 			ptr = packet->GetChunksPointer();

// 			while (ptr < buffer + length)
// 			{
// 				// Here we must anticipate the id of each chunk to use its appropriate.
// 				if (ptr + Chunk::ItemHeaderLength > packet->GetPaddingPointer())
// 				{
// 					MS_WARN_DEV("no space for chunk header");

// 					delete packet;
// 					return nullptr;
// 				}

// 				auto* chunk = Chunk::Parse(ptr, len - (ptr - data), /*exactLen*/ false);

// 				if (chunk)
// 				{
// 					ptr += chunk->GetSize();

// 					packet->AddChunk(chunk);
// 				}
// 				else
// 				{
// 					// TODO: Let' see.
// 					delete packet;
// 					return nullptr;
// 				}
// 			}

// 			size_t size = ptr - data;

// 			// Ensure computed size matches the total given length.
// 			if (size != len)
// 			{
// 				MS_WARN_TAG(sctp, "computed packet size does not match given length, packet discarded");

// 				delete packet;
// 				return nullptr;
// 			}

// 			packet->Parsed(size);

// 			return packet;
// 		}

// 		/* Instance methods. */

// 		Packet::Packet(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
// 		{
// 			MS_TRACE();
// 		}

// 		Packet::~Packet()
// 		{
// 			MS_TRACE();

// 			// TODO
// 			// for (auto* chunk : this->chunks)
// 			// {
// 			// 	delete chunk;
// 			// }
// 		}

// 		void Packet::Dump() const
// 		{
// 			MS_TRACE();

// 			MS_DUMP("<Packet>");
// 			MS_DUMP("  needs serialization: %s", NeedsSerialization() ? "true" : "false");
// 			MS_DUMP("  size: %zu", GetSize());
// 			MS_DUMP("  source port: %" PRIu16, GetSourcePort());
// 			MS_DUMP("  destination port: %" PRIu16, GetDestinationPort());
// 			MS_DUMP("  verification tag: %" PRIu32, GetVerificationTag());
// 			MS_DUMP("  checksum: %" PRIu32, GetChecksum());

// 			// TODO
// 			// for (auto* chunk : this->chunks)
// 			// {
// 			// 	chunk->Dump();
// 			// }

// 			MS_DUMP("</Packet>");
// 		}

// 		void Packet::Serialize(uint8_t* buffer, size_t bufferLength)
// 		{
// 			MS_TRACE();

// 			// TODO: Do this right.

// 			auto size = GetSize();

// 			// If already serialized we just need to copy current buffer to the new one
// 			// and adjust pointers.
// 			if (!NeedsSerialization())
// 			{
// 				// TODO

// 				// std::memcpy(buffer, GetCurrentBuffer(), size);

// 				// this->body   = buffer + (this->body - GetCurrentBuffer());
// 				// this->header = reinterpret_cast<Header*>(buffer);

// 				// Serialized(buffer, size);

// 				// return;
// 			}

// 			std::memcpy(buffer, GetCurrentBuffer(), size);

// 			Serialized(buffer, size);
// 		}
// 	} // namespace SCTP
// } // namespace RTC
