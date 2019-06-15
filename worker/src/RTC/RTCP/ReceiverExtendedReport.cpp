#define MS_CLASS "RTC::RTCP::ReceiverExtendedReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/ReceiverExtendedReport.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		ReceiverExtendedReport* ReceiverExtendedReport::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			if (header->blockType != 4)
			{
				MS_WARN_TAG(
				  rtcp,
				  "only support 4(Receiver Reference Time Report Block), current is %" PRIu8,
				  header->blockType);

				return nullptr;
			}

			// Packet size must be >= header size.
			if (sizeof(Header) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for receiver extended report, packet discarded");

				return nullptr;
			}

			return new ReceiverExtendedReport(header);
		}

		/* Instance methods. */

		void ReceiverExtendedReport::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ReceiverExtendedReport>");
			MS_DUMP("  ntp sec      : %" PRIu32, GetNtpSec());
			MS_DUMP("  ntp frac     : %" PRIu32, GetNtpFrac());
			MS_DUMP("</ReceiverExtendedReport>");
		}

		size_t ReceiverExtendedReport::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Copy the header.
			std::memcpy(buffer, this->header, sizeof(Header));

			return sizeof(Header);
		}

		/* Class methods. */

		/**
		 * ReceiverExtendedReportPacket::Parse()
		 * @param  data   - Points to the begining of the incoming RTCP packet.
		 * @param  len    - Total length of the packet.
		 * @param  offset - points to the first Receiver Extended Report if the incoming packet.
		 */
		ReceiverExtendedReportPacket* ReceiverExtendedReportPacket::Parse(
		  const uint8_t* data, size_t len, size_t offset)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common header and the SSRC of packet receiver.
			if (sizeof(CommonHeader) + sizeof(uint32_t) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for receiver extended report packet, packet discarded");

				return nullptr;
			}

			std::unique_ptr<ReceiverExtendedReportPacket> packet(new ReceiverExtendedReportPacket());

			packet->SetSsrc(
			  Utils::Byte::Get4Bytes(reinterpret_cast<uint8_t*>(header), sizeof(CommonHeader)));

			if (offset == 0)
				offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

			uint8_t count = header->count;

			if ((count == 0u) && (len > offset))
			{
				ReceiverExtendedReport* report = ReceiverExtendedReport::Parse(data + offset, len - offset);

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

		size_t ReceiverExtendedReportPacket::Serialize(uint8_t* buffer)
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

		void ReceiverExtendedReportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ReceiverExtendedReportPacket>");
			MS_DUMP("  ssrc: %" PRIu32, static_cast<uint32_t>(ntohl(this->ssrc)));
			for (auto* report : this->reports)
			{
				report->Dump();
			}
			MS_DUMP("</ReceiverExtendedReportPacket>");
		}
	} // namespace RTCP
} // namespace RTC
