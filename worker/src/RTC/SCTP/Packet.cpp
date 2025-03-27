#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Packet.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		Packet* Packet::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			if (!Packet::IsSctp(data, len))
			{
				MS_WARN_TAG(sctp, "not an SCTP packet");

				return nullptr;
			}

			auto* packet = new Packet(data, len);

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to other parts of the packet.
			auto* ptr = const_cast<uint8_t*>(data);

			// TODO: Move this to some Validate() method.
			if (packet->GetSourcePort() == 0u || packet->GetDestinationPort() == 0u)
			{
				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, packet discarded");

				delete packet;
				return nullptr;
			}

			// Inspect data after the minimum header size. Start looking for chunks
			// after SCTP Common Header.
			ptr += CommonHeaderSize;

			while (len > (ptr - data))
			{
				auto* chunk = Chunk::Parse(ptr, len - (ptr - data), /*exactLen*/ false);

				if (chunk)
				{
					ptr += chunk->GetSize();

					packet->AddChunk(chunk);
				}
				else
				{
					// TODO: Let' see.
					delete packet;
					return nullptr;
				}
			}

			// Ensure current position matches the total length.
			if (ptr - data != len)
			{
				MS_WARN_TAG(sctp, "computed packet size does not match total size, packet discarded");

				delete packet;
				return nullptr;
			}

			// TODO: Remove.
			packet->Dump();

			return packet;
		}

		/* Instance methods. */

		Packet::Packet(const uint8_t* buffer, size_t size)
		  : Serializable(buffer, size),
		    commonHeader(reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(buffer)))
		{
			MS_TRACE();
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

			MS_DUMP("  size: %zu", GetSize());

			MS_DUMP("  source port: %" PRIu16, GetSourcePort());

			MS_DUMP("  destination port: %" PRIu16, GetDestinationPort());

			MS_DUMP("  verification tag: %" PRIu32, GetVerificationTag());

			MS_DUMP("  checksum: %" PRIu32, GetChecksum());

			for (auto* chunk : this->chunks)
			{
				chunk->Dump();
			}

			MS_DUMP("</Packet>");
		}

		size_t Packet::GetSize() const
		{
			MS_TRACE();

			if (!NeedsSerialization())
			{
				return this->size;
				;
			}

			// TODO: Here we should really calculate the packet size.
			return this->size;
		}

		void Packet::Serialize(uint8_t* buffer, size_t size)
		{
			MS_TRACE();

			// TODO: Do this right.

			auto newSize = GetSize();

			std::memcpy(buffer, this->buffer, newSize);

			Serialized(buffer, newSize);
		}
	} // namespace SCTP
} // namespace RTC
