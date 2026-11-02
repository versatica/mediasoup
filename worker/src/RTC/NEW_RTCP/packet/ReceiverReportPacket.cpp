#define MS_CLASS "RTC::NEW_RTCP::ReceiverReportPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/ReceiverReportPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		ReceiverReportPacket* ReceiverReportPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::RR)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return ReceiverReportPacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		ReceiverReportPacket* ReceiverReportPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ReceiverReportPacket::ReceiverReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* packet = new ReceiverReportPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::RR, ReceiverReportPacket::ReceiverReportHeaderLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(ReceiverReportPacket::ReceiverReportHeaderLength);

			packet->SetSsrc(0);

			return packet;
		}

		ReceiverReportPacket* ReceiverReportPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			auto* packet = new ReceiverReportPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			if (!packet->IsLengthValid())
			{
				delete packet;
				return nullptr;
			}

			return packet;
		}

		/* Instance methods. */

		ReceiverReportPacket::ReceiverReportPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyReportPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		ReceiverReportPacket::~ReceiverReportPacket()
		{
			MS_TRACE();
		}

		void ReceiverReportPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::ReceiverReportPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			DumpReportBlocks(indentation);
			MS_DUMP_CLEAN(indentation, "</RTCP::ReceiverReportPacket>");
		}

		ReceiverReportPacket* ReceiverReportPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new ReceiverReportPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		ReceiverReportPacket* ReceiverReportPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedPacket = new ReceiverReportPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
