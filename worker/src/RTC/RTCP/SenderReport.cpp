#define MS_CLASS "RTC::RTCP::SenderReport"
#define MS_LOG_DEV

#include "RTC/RTCP/SenderReport.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	SenderReport* SenderReport::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Get the header.
		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			// Packet size must be >= header size.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for sender report, packet discarded");

			return nullptr;
		}

		return new SenderReport(header);
	}

	/* Instance methods. */

	void SenderReport::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<SenderReport>");
		MS_DUMP("  ssrc         : %" PRIu32, this->GetSsrc());
		MS_DUMP("  ntp sec      : %" PRIu32, this->GetNtpSec());
		MS_DUMP("  ntp frac     : %" PRIu32, this->GetNtpFrac());
		MS_DUMP("  rtp ts       : %" PRIu32, this->GetRtpTs());
		MS_DUMP("  packet count : %" PRIu32, this->GetPacketCount());
		MS_DUMP("  octet count  : %" PRIu32, this->GetOctetCount());
		MS_DUMP("</SenderReport>");
	}

	size_t SenderReport::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// Copy the header.
		std::memcpy(buffer, this->header, sizeof(Header));

		return sizeof(Header);
	}

	/* Class methods. */

	SenderReportPacket* SenderReportPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		std::unique_ptr<SenderReportPacket> packet(new SenderReportPacket());
		size_t offset = sizeof(Packet::CommonHeader);

		SenderReport* report = SenderReport::Parse(data+offset, len-offset);
		if (report)
			packet->AddReport(report);
		else
			return packet.release();

		return packet.release();
	}

	/* Instance methods. */

	size_t SenderReportPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		MS_ASSERT(this->reports.size() == 1, "invalid number of sender reports");

		size_t offset = Packet::Serialize(buffer);

		// Serialize reports.
		for (auto report : this->reports)
		{
			offset += report->Serialize(buffer + offset);
		}

		return offset;
	}

	void SenderReportPacket::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<SenderReportPacket>");
		for (auto report : this->reports)
		{
			report->Dump();
		}
		MS_DUMP("</SenderReportPacket>");
	}
}}
