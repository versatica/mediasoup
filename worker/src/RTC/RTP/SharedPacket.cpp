#define MS_CLASS "RTC::RTP::SharedPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/SharedPacket.hpp"
#include "Logger.hpp"
#include "RTC/Serializable.hpp"
#include <new> // std::align_val_t{

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		// When cloning a RTP packet, a buffer is allocated for it and its length is
		// the length of the packet plus this value (in bytes).
		static constexpr size_t PacketBufferLengthIncrement{ 100 };
#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
		// Total memory currently allocated by cloned RTP packets.
		static thread_local uint64_t allocatedMemory{ 0 };
#endif
		// Callback given to every cloned RTP packet to deallocate the buffer we
		// allocated for it.
		static thread_local Serializable::BufferReleasedListener PacketBufferReleasedListener =
		  // NOLINTNEXTLINE(misc-unused-parameters, readability-non-const-parameter)
		  [](const Serializable* packet, uint8_t* buffer)
		{
			// NOTE: Needed since we allocated it using
			::operator delete[](buffer, std::align_val_t{ 4 });

#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
			allocatedMemory -= packet->GetBufferLength();

			MS_DUMP_CLEAN(
			  0,
			  "[worker.pid:%" PRIu64
			  "] [RTC::RTP::SharedPacket] memory deallocated [packet buffer:%zu, total allocated memory:%" PRIu64
			  "]",
			  Logger::Pid,
			  packet->GetBufferLength(),
			  allocatedMemory);
#endif
		};

		/* Namespace methods. */

		SharedPacket ClonePacket(const RTP::Packet* packet)
		{
			MS_TRACE();

			const size_t bufferLength = packet->GetLength() + PacketBufferLengthIncrement;

			// NOTE: Buffer must be 4-byte aligned since RTP packet parsing casts it to
			// structs (e.g. FixedHeader, HeaderExtension) that require 4-byte alignment.
			auto* buffer = static_cast<uint8_t*>(::operator new[](bufferLength, std::align_val_t{ 4 }));
			auto* clonedPacket = packet->Clone(buffer, bufferLength);

			// Set a listener in the packet to deallocate the buffer above once the
			// packet releases it.
			clonedPacket->SetBufferReleasedListener(std::addressof(PacketBufferReleasedListener));

#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
			allocatedMemory += bufferLength;

			MS_DUMP_CLEAN(
			  0,
			  "[worker.pid:%" PRIu64
			  "] [RTC::RTP::SharedPacket] memory allocated [packet buffer:%zu, total allocated memory:%" PRIu64
			  "]",
			  Logger::Pid,
			  clonedPacket->GetBufferLength(),
			  allocatedMemory);
#endif

			// NOTE: The buffer is freed by PacketBufferReleasedListener above and not
			// by a custom deleter of the shared pointer. Deleting the cloned packet is
			// hence all this shared pointer must do, so the default deleter is used.
			return SharedPacket{ clonedPacket };
		}

		void AssertSamePacket(const SharedPacket& sharedPacket, const RTP::Packet* packet)
		{
			MS_TRACE();

			const auto* clonedPacket = sharedPacket.get();

			if (!clonedPacket && !packet)
			{
				return;
			}
			else if (clonedPacket && !packet)
			{
				MS_ABORT("there is a packet in sharedPacket but given packet doesn't have value");
			}
			else if (!clonedPacket && packet)
			{
				MS_ABORT("there is no packet in sharedPacket but given packet has value");
			}
			else
			{
				MS_ASSERT(
				  clonedPacket->GetSsrc() == packet->GetSsrc(),
				  "SSRC %" PRIu32 " in packet in sharedPacket != SSRC %" PRIu32 " in given packet",
				  clonedPacket->GetSsrc(),
				  packet->GetSsrc());

				MS_ASSERT(
				  clonedPacket->GetSequenceNumber() == packet->GetSequenceNumber(),
				  "seq %" PRIu16 " in packet in sharedPacket != seq %" PRIu16 " in given packet",
				  clonedPacket->GetSequenceNumber(),
				  packet->GetSequenceNumber());

				MS_ASSERT(
				  clonedPacket->GetTimestamp() == packet->GetTimestamp(),
				  "timestamp %" PRIu32 " in packet in sharedPacket != timestamp %" PRIu32 " in given packet",
				  clonedPacket->GetTimestamp(),
				  packet->GetTimestamp());

				MS_ASSERT(
				  clonedPacket->GetLength() == packet->GetLength(),
				  "length %zu of packet in sharedPacket != length %zu of given packet",
				  clonedPacket->GetLength(),
				  packet->GetLength());
			}
		}
	} // namespace RTP
} // namespace RTC
