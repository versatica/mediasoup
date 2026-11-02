#define MS_CLASS "RTC::NEW_RTCP::FirPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/FirPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		FirPacket* FirPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::PSFB)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			const auto messageType = static_cast<AnyPayloadSpecificFeedbackPacket::MessageType>(
			  reinterpret_cast<const Packet::CommonHeader*>(buffer)->count);

			if (messageType != AnyPayloadSpecificFeedbackPacket::MessageType::FIR)
			{
				MS_WARN_DEV("invalid message type");

				return nullptr;
			}

			return FirPacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		FirPacket* FirPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < AnyFeedbackPacket::FeedbackHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new FirPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::PSFB,
			  static_cast<uint8_t>(AnyPayloadSpecificFeedbackPacket::MessageType::FIR),
			  AnyFeedbackPacket::FeedbackHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSenderSsrc(0);
			packet->SetMediaSsrc(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum FirPacket length.

			return packet;
		}

		FirPacket* FirPacket::ParseStrict(const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			// The Feedback Control Information (FCI) must contain at least one item.
			if (packetLength < AnyFeedbackPacket::FeedbackHeaderLength + FirPacket::FirItemLength)
			{
				MS_WARN_TAG(rtcp, "no space for feedback header and at least one item");

				return nullptr;
			}

			// The Feedback Control Information (FCI) must contain whole items.
			if ((packetLength - AnyFeedbackPacket::FeedbackHeaderLength) % FirPacket::FirItemLength != 0)
			{
				MS_WARN_TAG(
				  rtcp,
				  "Feedback Control Information length (%zu bytes) is not multiple of %zu",
				  packetLength - AnyFeedbackPacket::FeedbackHeaderLength,
				  FirPacket::FirItemLength);

				return nullptr;
			}

			auto* const packet = new FirPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		FirPacket::FirPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyPayloadSpecificFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AnyFeedbackPacket::FeedbackHeaderLength);
		}

		FirPacket::~FirPacket()
		{
			MS_TRACE();
		}

		void FirPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::FirPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyPayloadSpecificFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  items: %zu", GetNumberOfItems());
			for (const auto& item : GetItems())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - ssrc: %" PRIu32 ", sequence number: %" PRIu8,
				  item.ssrc,
				  item.sequenceNumber);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::FirPacket>");
		}

		FirPacket* FirPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new FirPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		std::vector<FirPacket::Item> FirPacket::GetItems() const
		{
			MS_TRACE();

			const auto* const itemsPointer = GetItemsPointer();
			const size_t numberOfItems     = GetNumberOfItems();

			std::vector<Item> items;

			items.reserve(numberOfItems);

			for (size_t idx{ 0 }; idx < numberOfItems; ++idx)
			{
				const auto* const ptr = itemsPointer + (idx * FirPacket::FirItemLength);

				items.push_back(
				  { .ssrc = Utils::Byte::Get4Bytes(ptr, 0), .sequenceNumber = Utils::Byte::Get1Byte(ptr, 4) });
			}

			return items;
		}

		void FirPacket::AddItem(uint32_t ssrc, uint8_t sequenceNumber)
		{
			MS_TRACE();

			// The new item goes right after the last one.
			auto* const ptr = GetItemsPointer() + (GetNumberOfItems() * FirPacket::FirItemLength);

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + FirPacket::FirItemLength);

			// Write the item, with the reserved bits set to 0.
			Utils::Byte::Set4Bytes(ptr, 0, ssrc);
			Utils::Byte::Set1Byte(ptr, 4, sequenceNumber);
			Utils::Byte::Set3Bytes(ptr, 5, 0);
		}

		FirPacket* FirPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new FirPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
