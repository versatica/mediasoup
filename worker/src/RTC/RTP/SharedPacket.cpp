#define MS_CLASS "RTC::RTP::SharedPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/SharedPacket.hpp"
#include "Logger.hpp"
#include "RTC/Serializable.hpp"

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		// When cloning a RTP packet, a buffer is allocated for it and its length is
		// the length of the Packet plus this value (in bytes).
		static constexpr size_t PacketBufferLengthIncrement{ 100 };
		// Callback to pass to every cloned RTP Packet to deallocate its buffer once
		// the Packet releases its buffer (for example when the Packet is destroyed).
		static thread_local Serializable::BufferReleasedListener PacketBufferReleasedListener =
		  [](const Serializable* packet, uint8_t* buffer)
		{
			delete[] buffer;

#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
			SharedPacket::allocatedMemory -= packet->GetBufferLength();

			MS_DUMP_CLEAN(
			  0,
			  "[RTC::RTP::SharedPacket] memory deallocated [packet buffer:%zu, total allocated memory:%" PRIu64
			  "]",
			  packet->GetBufferLength(),
			  SharedPacket::allocatedMemory);
#endif
		};

		/* Class variables. */

#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
		thread_local uint64_t SharedPacket::allocatedMemory{ 0 };
#endif

		/* Instance methods. */

		SharedPacket::SharedPacket()
		  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RTP::Packet>>(nullptr))
		{
			MS_TRACE();
		}

		SharedPacket::SharedPacket(RTC::RTP::Packet* packet)
		  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RTP::Packet>>(nullptr))
		{
			MS_TRACE();

			if (packet)
			{
				StorePacket(packet);
			}
		}

		void SharedPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SharedPacket>");
			MS_DUMP_CLEAN(indentation, "  has packet: %s", HasPacket() ? "yes" : "no");
			if (HasPacket())
			{
				const auto* packet = GetPacket();

				packet->Dump(indentation + 1);
			}
			MS_DUMP_CLEAN(indentation, "</SharedPacket>");
		}

		void SharedPacket::Assign(RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			if (packet)
			{
				StorePacket(packet);
			}
			else
			{
				this->sharedPtr->reset(nullptr);
			}
		}

		void SharedPacket::Reset()
		{
			MS_TRACE();

			this->sharedPtr->reset(nullptr);
		}

		void SharedPacket::AssertSamePacket(const RTC::RTP::Packet* otherPacket) const
		{
			MS_TRACE();

			const auto* packet = GetPacket();

			if (!packet && !otherPacket)
			{
				return;
			}
			else if (packet && !otherPacket)
			{
				MS_ABORT("there is a packet in sharedPacket but given otherPacket doesn't have value");
			}
			else if (!packet && otherPacket)
			{
				MS_ABORT("there is no packet in sharedPacket but given otherPacket has value");
			}
			else
			{
				MS_ASSERT(
				  packet->GetSsrc() == otherPacket->GetSsrc(),
				  "SSRC %" PRIu32 " in packet in sharedPacket != SSRC %" PRIu32 " in otherPacket",
				  packet->GetSsrc(),
				  otherPacket->GetSsrc());

				MS_ASSERT(
				  packet->GetSequenceNumber() == otherPacket->GetSequenceNumber(),
				  "seq %" PRIu16 " in packet in sharedPacket != seq %" PRIu16 " in otherPacket",
				  packet->GetSequenceNumber(),
				  otherPacket->GetSequenceNumber());

				MS_ASSERT(
				  packet->GetTimestamp() == otherPacket->GetTimestamp(),
				  "timestamp %" PRIu16 " in packet in sharedPacket != timestamp %" PRIu16 " in otherPacket",
				  packet->GetTimestamp(),
				  otherPacket->GetTimestamp());

				MS_ASSERT(
				  packet->GetLength() == otherPacket->GetLength(),
				  "length %zu of packet in sharedPacket != length %zu of otherPacket",
				  packet->GetLength(),
				  otherPacket->GetLength());
			}
		}

		void SharedPacket::StorePacket(RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			const size_t bufferLength = packet->GetLength() + PacketBufferLengthIncrement;
			auto* buffer              = new uint8_t[bufferLength];
			auto* clonedPacket        = packet->Clone(buffer, bufferLength);

			// Set a listener in the Packet to deallocate its buffer once the Packet
			// is destroyed or releases its internal buffer.
			clonedPacket->SetBufferReleasedListener(std::addressof(PacketBufferReleasedListener));

			this->sharedPtr->reset(clonedPacket);

#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
			SharedPacket::allocatedMemory += bufferLength;

			MS_DUMP_CLEAN(
			  0,
			  "[RTC::RTP::SharedPacket] memory allocated [packet buffer:%zu, total allocated memory:%" PRIu64
			  "]",
			  clonedPacket->GetBufferLength(),
			  SharedPacket::allocatedMemory);
#endif
		}
	} // namespace RTP
} // namespace RTC
