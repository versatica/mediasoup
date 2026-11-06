#define MS_CLASS "RTC::NEW_RTCP::NackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/NackPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		NackPacket* NackPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::RTPFB)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			const auto messageType =
			  static_cast<AnyTransportLayerFeedbackPacket::MessageType>(buffer[0] & 0x1F);

			if (messageType != AnyTransportLayerFeedbackPacket::MessageType::NACK)
			{
				MS_WARN_DEV("invalid message type");

				return nullptr;
			}

			return NackPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		NackPacket* NackPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < AnyFeedbackPacket::FeedbackHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new NackPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::RTPFB,
			  static_cast<uint8_t>(AnyTransportLayerFeedbackPacket::MessageType::NACK),
			  AnyFeedbackPacket::FeedbackHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSenderSsrc(0);
			packet->SetMediaSsrc(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum NackPacket length.

			return packet;
		}

		size_t NackPacket::ComputeLengthFor(size_t numItems)
		{
			MS_TRACE();

			// The items go right after the feedback header, and each one is a
			// multiple of 4 bytes, so the packet never needs padding.
			return AnyFeedbackPacket::FeedbackHeaderLength + (numItems * NackPacket::NackItemLength);
		}

		NackPacket* NackPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			// The Feedback Control Information (FCI) must contain at least one item.
			if (contentLength < AnyFeedbackPacket::FeedbackHeaderLength + NackPacket::NackItemLength)
			{
				MS_WARN_TAG(rtcp, "no space for feedback header and at least one item");

				return nullptr;
			}

			// The Feedback Control Information (FCI) must contain whole items.
			if ((contentLength - AnyFeedbackPacket::FeedbackHeaderLength) % NackPacket::NackItemLength != 0)
			{
				MS_WARN_TAG(
				  rtcp,
				  "Feedback Control Information length (%zu bytes) is not multiple of %zu",
				  contentLength - AnyFeedbackPacket::FeedbackHeaderLength,
				  NackPacket::NackItemLength);

				return nullptr;
			}

			auto* const packet = new NackPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		NackPacket::NackPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyTransportLayerFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AnyFeedbackPacket::FeedbackHeaderLength);
		}

		NackPacket::~NackPacket()
		{
			MS_TRACE();
		}

		void NackPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::NackPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyTransportLayerFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  items: %zu", GetNumberOfItems());
			for (const auto& item : GetItems())
			{
				MS_DUMP_CLEAN(
				  indentation, "  - pid: %" PRIu16 ", blp: 0x%04" PRIX16, item.packetId, item.lostPacketBitmask);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::NackPacket>");
		}

		NackPacket* NackPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new NackPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		std::vector<NackPacket::Item> NackPacket::GetItems() const
		{
			MS_TRACE();

			const auto* const itemsPointer = GetItemsPointer();
			const size_t numberOfItems     = GetNumberOfItems();

			std::vector<Item> items;

			items.reserve(numberOfItems);

			for (size_t idx{ 0 }; idx < numberOfItems; ++idx)
			{
				const auto* const ptr = itemsPointer + (idx * NackPacket::NackItemLength);

				items.push_back(
				  { .packetId          = Utils::Byte::Get2Bytes(ptr, 0),
					  .lostPacketBitmask = Utils::Byte::Get2Bytes(ptr, 2) });
			}

			return items;
		}

		void NackPacket::AddItem(uint16_t packetId, uint16_t lostPacketBitmask)
		{
			MS_TRACE();

			// The new item goes right after the last one.
			auto* const ptr = GetItemsPointer() + (GetNumberOfItems() * NackPacket::NackItemLength);

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + NackPacket::NackItemLength);

			// Write the item.
			Utils::Byte::Set2Bytes(ptr, 0, packetId);
			Utils::Byte::Set2Bytes(ptr, 2, lostPacketBitmask);
		}

		NackPacket* NackPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new NackPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
