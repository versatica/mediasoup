#define MS_CLASS "RTC::RTCP::SenderExtendedReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/SenderExtendedReport.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		SenderExtendedReport* SenderExtendedReport::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			if (header->blockType != 5)
			{
				MS_WARN_TAG(rtcp, "only support 5(DLRR Report Block), current is %hhu", header->blockType);

				return nullptr;
			}

			// Packet size must be >= header size.
			if (sizeof(Header) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for sender extended report, packet discarded");

				return nullptr;
			}

			return new SenderExtendedReport(header);
		}

		/* Instance methods. */

		void SenderExtendedReport::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SenderExtendedReport>");
			MS_DUMP("  block type   : %" PRIu8, GetBlockType());
			MS_DUMP("  reserved     : %" PRIu8, GetReserved());
			MS_DUMP("  block length : %" PRIu16, GetBlockLength());
			MS_DUMP("  ssrc         : %" PRIu32, GetSsrc());
			MS_DUMP("  lrr          : %" PRIu32, GetLastReceiverReport());
			MS_DUMP("  dlrr         : %" PRIu32, GetDelaySinceLastReceiverReport());
			MS_DUMP("</SenderExtendedReport>");
		}

		size_t SenderExtendedReport::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Copy the header.
			std::memcpy(buffer, this->header, sizeof(Header));

			return sizeof(Header);
		}

		/* Class methods. */

		/**
		 * SenderExtendedReportPacket::Parse()
		 * @param  data   - Points to the begining of the incoming RTCP packet.
		 * @param  len    - Total length of the packet.
		 * @param  offset - points to the first Sender Extended Report if the incoming packet.
		 */
		SenderExtendedReportPacket* SenderExtendedReportPacket::Parse(
		  const uint8_t* data, size_t len, size_t offset)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common header and the SSRC of packet sender.
			if (sizeof(CommonHeader) + sizeof(uint32_t) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for sender extended report packet, packet discarded");

				return nullptr;
			}

			std::unique_ptr<SenderExtendedReportPacket> packet(new SenderExtendedReportPacket());

			packet->SetSsrc(
			  Utils::Byte::Get4Bytes(reinterpret_cast<uint8_t*>(header), sizeof(CommonHeader)));

			if (offset == 0)
				offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

			uint8_t count = header->count;

			if ((count == 0u) && (len > offset))
			{
				SenderExtendedReport* report = SenderExtendedReport::Parse(data + offset, len - offset);

				if (report != nullptr)
				{
					packet->AddReport(report);
					offset += report->GetSize();
				}
				else
				{
					return packet.release();
				}
			}

			return packet.release();
		}

		/* Instance methods. */

		size_t SenderExtendedReportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			size_t offset = Packet::Serialize(buffer);

			// Copy the SSRC.
			std::memcpy(buffer + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
			offset += sizeof(this->ssrc);

			// Serialize reports.
			for (auto* report : this->reports)
			{
				offset += report->Serialize(buffer + offset);
			}

			return offset;
		}

		void SenderExtendedReportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SenderExtendedReportPacket>");
			MS_DUMP("  ssrc: %" PRIu32, static_cast<uint32_t>(ntohl(this->ssrc)));
			for (auto* report : this->reports)
			{
				report->Dump();
			}
			MS_DUMP("</SenderExtendedReportPacket>");
		}
	} // namespace RTCP
} // namespace RTC
