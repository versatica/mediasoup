#define MS_CLASS "RTC::NEW_RTCP::PliPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/PliPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		PliPacket* PliPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::PSFB)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			const auto messageType =
			  static_cast<AnyPayloadSpecificFeedbackPacket::MessageType>(buffer[0] & 0x1F);

			if (messageType != AnyPayloadSpecificFeedbackPacket::MessageType::PLI)
			{
				MS_WARN_DEV("invalid message type");

				return nullptr;
			}

			return PliPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		PliPacket* PliPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < AnyFeedbackPacket::FeedbackHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new PliPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::PSFB,
			  static_cast<uint8_t>(AnyPayloadSpecificFeedbackPacket::MessageType::PLI),
			  AnyFeedbackPacket::FeedbackHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSenderSsrc(0);
			packet->SetMediaSsrc(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum PliPacket length.

			return packet;
		}

		PliPacket* PliPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			// PLI has no FCI.
			if (contentLength != AnyFeedbackPacket::FeedbackHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "PliPacket length (excluding padding) must be %zu (got %zu)",
				  AnyFeedbackPacket::FeedbackHeaderLength,
				  contentLength);

				return nullptr;
			}

			auto* const packet = new PliPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		PliPacket::PliPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyPayloadSpecificFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AnyFeedbackPacket::FeedbackHeaderLength);
		}

		PliPacket::~PliPacket()
		{
			MS_TRACE();
		}

		void PliPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::PliPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyPayloadSpecificFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(indentation, "</RTCP::PliPacket>");
		}

		PliPacket* PliPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new PliPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		PliPacket* PliPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new PliPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
