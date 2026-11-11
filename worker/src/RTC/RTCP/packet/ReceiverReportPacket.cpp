#define MS_CLASS "RTC::RTCP::ReceiverReportPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		ReceiverReportPacket* ReceiverReportPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::RR)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return ReceiverReportPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		ReceiverReportPacket* ReceiverReportPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ReceiverReportPacket::ReceiverReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new ReceiverReportPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::RR, 0, ReceiverReportPacket::ReceiverReportHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSsrc(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum ReceiverReportPacket length.

			return packet;
		}

		size_t ReceiverReportPacket::ComputeLengthFor(size_t numReportBlocks)
		{
			MS_TRACE();

			// The report blocks go right after the fixed header, and each one is a
			// multiple of 4 bytes, so the packet never needs padding.
			return ReceiverReportPacket::ReceiverReportHeaderLength +
			       (numReportBlocks * AnyReportPacket::ReportBlockLength);
		}

		ReceiverReportPacket* ReceiverReportPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < ReceiverReportPacket::ReceiverReportHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "ReceiverReportPacket length must be equal or greater than %zu",
				  ReceiverReportPacket::ReceiverReportHeaderLength);

				return nullptr;
			}

			auto* const packet = new ReceiverReportPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			// Validate that the announced report blocks fit into the packet.
			const size_t reportBlocksEndOffset =
			  ReceiverReportPacket::ReceiverReportHeaderLength +
			  (packet->GetNumberOfReportBlocks() * AnyReportPacket::ReportBlockLength);

			if (reportBlocksEndOffset > contentLength)
			{
				MS_WARN_TAG(rtcp, "report blocks exceed the packet length");

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

			SetLength(ReceiverReportPacket::ReceiverReportHeaderLength);
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

			auto* const clonedPacket = new ReceiverReportPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		ReceiverReportPacket* ReceiverReportPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket =
			  new ReceiverReportPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace RTCP
} // namespace RTC
