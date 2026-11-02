#define MS_CLASS "RTC::NEW_RTCP::SenderReportPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/SenderReportPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		SenderReportPacket* SenderReportPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::SR)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return SenderReportPacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		SenderReportPacket* SenderReportPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < SenderReportPacket::SenderReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* packet = new SenderReportPacket(buffer, bufferLength);

			packet->InitializeHeader(Packet::PacketType::SR, SenderReportPacket::SenderReportHeaderLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(SenderReportPacket::SenderReportHeaderLength);

			packet->SetSsrc(0);
			packet->SetNtpSec(0);
			packet->SetNtpFrac(0);
			packet->SetRtpTimestamp(0);
			packet->SetPacketCount(0);
			packet->SetOctetCount(0);

			return packet;
		}

		SenderReportPacket* SenderReportPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			auto* packet = new SenderReportPacket(const_cast<uint8_t*>(buffer), bufferLength);

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

		SenderReportPacket::SenderReportPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyReportPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		SenderReportPacket::~SenderReportPacket()
		{
			MS_TRACE();
		}

		void SenderReportPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::SenderReportPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			MS_DUMP_CLEAN(indentation, "  ntp sec: %" PRIu32, GetNtpSec());
			MS_DUMP_CLEAN(indentation, "  ntp frac: %" PRIu32, GetNtpFrac());
			MS_DUMP_CLEAN(indentation, "  rtp timestamp: %" PRIu32, GetRtpTimestamp());
			MS_DUMP_CLEAN(indentation, "  packet count: %" PRIu32, GetPacketCount());
			MS_DUMP_CLEAN(indentation, "  octet count: %" PRIu32, GetOctetCount());
			DumpReportBlocks(indentation);
			MS_DUMP_CLEAN(indentation, "</RTCP::SenderReportPacket>");
		}

		SenderReportPacket* SenderReportPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new SenderReportPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		void SenderReportPacket::SetNtpSec(uint32_t ntpSec)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, ntpSec);
		}

		void SenderReportPacket::SetNtpFrac(uint32_t ntpFrac)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, ntpFrac);
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

			auto* softClonedPacket = new SenderReportPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
