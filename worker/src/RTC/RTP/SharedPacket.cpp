#define MS_CLASS "RTC::RTP::SharedPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/SharedPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		// When cloning a RTP packet, a buffer is allocated for it and its length is
		// the length of the Packet plus this value (in bytes).
		static constexpr size_t PacketBufferLengthIncrement{ 100 };

		/* Instance methods. */

		SharedPacket::SharedPacket()
		  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RTP::Packet>>(nullptr))
		{
			MS_TRACE();
		}

		SharedPacket::SharedPacket(const RTC::RTP::Packet* packet)
		  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RTP::Packet>>(nullptr))
		{
			MS_TRACE();

			if (packet)
			{
				const size_t bufferLength = packet->GetLength() + PacketBufferLengthIncrement;
				auto* buffer              = new uint8_t[bufferLength];

				this->sharedPtr->reset(packet->Clone(buffer, bufferLength));
			}
		}

		SharedPacket::~SharedPacket()
		{
			MS_TRACE();

			// If we hold a Packet we must delete its internal buffer (the one we
			// passed to it via Clone() method).
			if (HasPacket())
			{
				delete[] GetPacket()->GetBuffer();
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

		void SharedPacket::Assign(const RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			// If we hold a Packet we must delete its internal buffer (the one we
			// passed to it via Clone() method).
			if (HasPacket())
			{
				delete[] GetPacket()->GetBuffer();
			}

			if (packet)
			{
				const size_t bufferLength = packet->GetLength() + PacketBufferLengthIncrement;
				auto* buffer              = new uint8_t[bufferLength];

				this->sharedPtr->reset(packet->Clone(buffer, bufferLength));
			}
			else
			{
				this->sharedPtr->reset(nullptr);
			}
		}

		void SharedPacket::Reset()
		{
			MS_TRACE();

			// If we hold a Packet we must delete its internal buffer (the one we
			// passed to it via Clone() method).
			if (HasPacket())
			{
				delete[] GetPacket()->GetBuffer();
			}

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
	} // namespace RTP
} // namespace RTC
