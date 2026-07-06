#define MS_CLASS "RTC::RTCP::CompoundPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/CompoundPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		CompoundPacket* CompoundPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!Packet::IsRtcp(buffer, bufferLength))
			{
				MS_WARN_TAG(rtcp, "not an RTCP compound packet");

				return nullptr;
			}

			auto* compoundPacket = new CompoundPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to each packet within the compound packet.
			const auto* ptr = buffer;

			while (ptr < buffer + bufferLength)
			{
				// The remaining length in the buffer is the potential buffer length
				// of the current packet.
				const size_t packetMaxBufferLength = bufferLength - (ptr - buffer);

				// Here we must anticipate the type of each packet to use its appropriate
				// parser.
				Packet::PacketType packetType;
				size_t packetLength;

				if (!Packet::IsPacket(ptr, packetMaxBufferLength, packetType, packetLength))
				{
					MS_WARN_TAG(rtcp, "not an RTCP packet");

					delete compoundPacket;
					return nullptr;
				}

				Packet* packet{ nullptr }; // NOLINT(misc-const-correctness)

				MS_DEBUG_DEV("parsing RTCP packet [ptr:%zu, type:%" PRIu8 "]", ptr - buffer, packetType);

				switch (packetType)
				{
					case Packet::PacketType::IJ:
					{
						// TODO
						// packet = ExtendedJitterReportPacket::ParseStrict(ptr, packetLength);

						break;
					}

					default:
					{
						// TODO
						// packet = UnknownPacket::ParseStrict(ptr, packetLength);
					}
				}

				if (!packet)
				{
					delete compoundPacket;
					return nullptr;
				}

				compoundPacket->packets.push_back(packet);

				ptr += packet->GetLength();
			}

			const size_t computedLength = ptr - buffer;

			// Ensure computed length matches the total given buffer length.
			if (computedLength != bufferLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "computed length (%zu bytes) != buffer length (%zu bytes)",
				  computedLength,
				  bufferLength);

				delete compoundPacket;
				return nullptr;
			}

			// It's mandatory to call SetLength() once we are done and we know the
			// exact length of the packet.
			compoundPacket->SetLength(computedLength);

			return compoundPacket;
		}

		/* Instance methods. */

		CompoundPacket::CompoundPacket(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		CompoundPacket::~CompoundPacket()
		{
			MS_TRACE();
		}

		void CompoundPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::CompoundPacket>");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  packets count: %zu", GetPacketsCount());
			MS_DUMP_CLEAN(
			  indentation, "  needs consolidation of chunks: %s", NeedsConsolidation() ? "yes" : "no");
			for (const auto* packet : this->packets)
			{
				packet->Dump(indentation + 1);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::CompoundPacket>");
		}

		void CompoundPacket::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			for (auto* packet : this->packets)
			{
				const size_t offset = packet->GetBuffer() - previousBuffer;

				packet->SoftSerialize(buffer + offset);
			}
		}

		CompoundPacket* CompoundPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedCompoundPacket = new CompoundPacket(buffer, bufferLength);

			Serializable::CloneInto(clonedCompoundPacket);

			// Soft clone packets into the given cloned compound packet.
			for (auto* packet : this->packets)
			{
				const size_t offset = packet->GetBuffer() - GetBuffer();

				auto* softClonedPacket = packet->SoftClone(buffer + offset);

				clonedCompoundPacket->packets.push_back(softClonedPacket);
			}

			return clonedCompoundPacket;
		}

		void CompoundPacket::HandleInPlacePacket(Packet* packet)
		{
			MS_TRACE();

			this->needsConsolidation = true;

			// When the application completes the packet it must call
			// `packet->Consolidate()` and that will trigger this event.
			packet->SetConsolidatedListener(
			  [this, packet]()
			  {
				  try
				  {
					  // NOTE: Packet class doesn't have `NeedsConsolidation()` method.

					  // Fix buffer length assigned to the packet.
					  packet->SetBufferLength(packet->GetLength());

					  // Update compound packet length.
					  // NOTE: This will throw if there is no enough space in the compound
					  // packet buffer.
					  SetLength(GetLength() + packet->GetLength());

					  // Add the packet to the list.
					  this->packets.push_back(packet);
					  this->needsConsolidation = false;
				  }
				  catch (const MediaSoupError& error)
				  {
					  this->needsConsolidation = false;

					  throw;
				  }
			  });
		}

		void CompoundPacket::AssertDoesNotNeedConsolidation() const
		{
			MS_TRACE();

			if (this->needsConsolidation)
			{
				MS_THROW_ERROR("compound packet needs consolidation of some ongoing packet");
			}
		}
	} // namespace NEW_RTCP
} // namespace RTC
