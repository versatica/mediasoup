#define MS_CLASS "RTC::RTCP::SenderReportPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/SenderReportPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		SenderReportPacket* SenderReportPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::SR)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return SenderReportPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		SenderReportPacket* SenderReportPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < SenderReportPacket::SenderReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new SenderReportPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::SR, 0, SenderReportPacket::SenderReportHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSsrc(0);
			packet->SetNtp({ .seconds = 0, .fractions = 0 });
			packet->SetRtpTimestamp(0);
			packet->SetPacketCount(0);
			packet->SetOctetCount(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum SenderReportPacket length.

			return packet;
		}

		size_t SenderReportPacket::ComputeLengthFor(size_t numReportBlocks)
		{
			MS_TRACE();

			// The report blocks go right after the fixed header, and each one is a
			// multiple of 4 bytes, so the packet never needs padding.
			return SenderReportPacket::SenderReportHeaderLength +
			       (numReportBlocks * AnyReportPacket::ReportBlockLength);
		}

		SenderReportPacket* SenderReportPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < SenderReportPacket::SenderReportHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "SenderReportPacket length must be equal or greater than %zu",
				  SenderReportPacket::SenderReportHeaderLength);

				return nullptr;
			}

			auto* const packet = new SenderReportPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			// Validate that the announced report blocks fit into the packet.
			const size_t reportBlocksEndOffset =
			  SenderReportPacket::SenderReportHeaderLength +
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

		SenderReportPacket::SenderReportPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyReportPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(SenderReportPacket::SenderReportHeaderLength);
		}

		SenderReportPacket::~SenderReportPacket()
		{
			MS_TRACE();
		}

		void SenderReportPacket::Dump(int indentation) const
		{
			MS_TRACE();

			const Utils::Time::Ntp ntp = GetNtp();

			MS_DUMP_CLEAN(indentation, "<RTCP::SenderReportPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			MS_DUMP_CLEAN(indentation, "  ntp sec: %" PRIu32, ntp.seconds);
			MS_DUMP_CLEAN(indentation, "  ntp frac: %" PRIu32, ntp.fractions);
			MS_DUMP_CLEAN(indentation, "  rtp timestamp: %" PRIu32, GetRtpTimestamp());
			MS_DUMP_CLEAN(indentation, "  packet count: %" PRIu32, GetPacketCount());
			MS_DUMP_CLEAN(indentation, "  octet count: %" PRIu32, GetOctetCount());
			DumpReportBlocks(indentation);
			MS_DUMP_CLEAN(indentation, "</RTCP::SenderReportPacket>");
		}

		SenderReportPacket* SenderReportPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new SenderReportPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		void SenderReportPacket::SetNtp(const Utils::Time::Ntp& ntp)
		{
			MS_TRACE();

			auto* const buffer = const_cast<uint8_t*>(GetBuffer());

			Utils::Byte::Set4Bytes(buffer, 8, ntp.seconds);
			Utils::Byte::Set4Bytes(buffer, 12, ntp.fractions);
		}

		void SenderReportPacket::SetRtpTimestamp(uint32_t rtpTimestamp)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, rtpTimestamp);
		}

		void SenderReportPacket::SetPacketCount(uint32_t packetCount)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 20, packetCount);
		}

		void SenderReportPacket::SetOctetCount(uint32_t octetCount)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 24, octetCount);
		}

		SenderReportPacket* SenderReportPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket =
			  new SenderReportPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace RTCP
} // namespace RTC
