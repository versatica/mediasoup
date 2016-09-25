#define MS_CLASS "RTC::RTCP::ReceiverReport"

#include "RTC/RTCP/ReceiverReport.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{
	/* ReceiverReport Class methods. */

	ReceiverReport* ReceiverReport::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		// Get the header.
		Header* header = (Header*)data;

		// Packet size must be >= header size.
		if (sizeof(Header) > len)
		{
				MS_WARN("not enough space for receiver report, packet discarded");
				return nullptr;
		}

		return new ReceiverReport(header);
	}

	/* ReceiverReport Instance methods. */

	void ReceiverReport::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<ReceiverReport>");
		MS_WARN("\t\tssrc: %u", ntohl(this->header->ssrc));
		MS_WARN("\t\tfraction_lost: %u", this->header->fraction_lost);
		MS_WARN("\t\ttotal_lost: %d", this->GetTotalLost());
		MS_WARN("\t\tlast_sec: %u", ntohl(this->header->last_sec));
		MS_WARN("\t\tjitter: %u", ntohl(this->header->jitter));
		MS_WARN("\t\tlsr: %u", ntohl(this->header->lsr));
		MS_WARN("\t\tdlsr: %u", ntohl(this->header->dlsr));
		MS_WARN("\t</ReceiverReport>");
	}

	void ReceiverReport::Serialize()
	{
		MS_TRACE_STD();

		// Allocate internal data.
		if (!this->raw) {
			this->raw = new uint8_t[sizeof(Header)];

			// Copy the header.
			std::memcpy(this->raw, this->header, sizeof(Header));

			// Update the header pointer.
			this->header = (Header*)(this->raw);
		}
	}

	size_t ReceiverReport::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		// Copy the header.
		std::memcpy(data, this->header, sizeof(Header));

		return sizeof(Header);
	}

	/* ReceiverReportPacket Class methods. */

	/* @data points to the begining of the incoming RTCP packet
	 * @len is the total length of the packet
	 * @offset points to the first Receiver Report if the incoming packet
	 */
	ReceiverReportPacket* ReceiverReportPacket::Parse(const uint8_t* data, size_t len, size_t offset)
	{
		MS_TRACE_STD();

		// Get the header.
		Packet::CommonHeader* header = (Packet::CommonHeader*)data;

		std::auto_ptr<ReceiverReportPacket> packet(new ReceiverReportPacket());

		packet->SetSsrc(Utils::Byte::Get4Bytes((uint8_t*)header, sizeof(CommonHeader)));

		if (offset == 0)
		{
			offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;
		}

		uint8_t count = header->count;
		while ((count--) && (len - offset > 0))
		{
			ReceiverReport* report = ReceiverReport::Parse(data+offset, len-offset);
			if (report) {
				packet->AddReport(report);
				offset += report->GetSize();
			} else {
				return packet.release();
			}
		}

		return packet.release();
	}

	/* ReceiverReportPacket Instance methods. */

	size_t ReceiverReportPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = Packet::Serialize(data);

		// Copy the SSRC
		std::memcpy(data + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
		offset += sizeof(this->ssrc);

		// Serialize reports
		for(auto report : this->reports) {
			offset += report->Serialize(data + offset);
		}

		return offset;
	}

	void ReceiverReportPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("<ReceiverReportPacket>");
		MS_WARN("\tssrc: %u", (uint32_t)ntohl(this->ssrc));

		for(auto report : this->reports) {
			report->Dump();
		}

		MS_WARN("</ReceiverReportPacket>");
	}
}
}
