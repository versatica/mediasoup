#define MS_CLASS "RTC::NEW_RTCP::UnknownTransportLayerFeedbackPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/UnknownTransportLayerFeedbackPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		UnknownTransportLayerFeedbackPacket* UnknownTransportLayerFeedbackPacket::Parse(
		  const uint8_t* buffer, size_t bufferLength)
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

			return UnknownTransportLayerFeedbackPacket::ParseStrict(
			  buffer, bufferLength, packetLength, padding);
		}

		UnknownTransportLayerFeedbackPacket* UnknownTransportLayerFeedbackPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < AnyFeedbackPacket::FeedbackHeaderLength)
			{
				MS_WARN_TAG(rtcp, "no space for feedback header");

				return nullptr;
			}

			auto* const packet =
			  new UnknownTransportLayerFeedbackPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		UnknownTransportLayerFeedbackPacket::UnknownTransportLayerFeedbackPacket(
		  uint8_t* buffer, size_t bufferLength)
		  : AnyTransportLayerFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(AnyFeedbackPacket::FeedbackHeaderLength);
		}

		UnknownTransportLayerFeedbackPacket::~UnknownTransportLayerFeedbackPacket()
		{
			MS_TRACE();
		}

		void UnknownTransportLayerFeedbackPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::UnknownTransportLayerFeedbackPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyTransportLayerFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unknown value length: %zu (has unknown value: %s)",
			  GetUnknownValueLength(),
			  HasUnknownValue() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</RTCP::UnknownTransportLayerFeedbackPacket>");
		}

		UnknownTransportLayerFeedbackPacket* UnknownTransportLayerFeedbackPacket::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new UnknownTransportLayerFeedbackPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		UnknownTransportLayerFeedbackPacket* UnknownTransportLayerFeedbackPacket::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket =
			  new UnknownTransportLayerFeedbackPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
