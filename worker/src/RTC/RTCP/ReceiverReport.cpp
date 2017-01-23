#define MS_CLASS "RTC::RTCP::ReceiverReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/ReceiverReport.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	ReceiverReport* ReceiverReport::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Get the header.
		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

		// Packet size must be >= header size.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for receiver report, packet discarded");

			return nullptr;
		}

		return new ReceiverReport(header);
	}

	/* Instance methods. */

	void ReceiverReport::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<ReceiverReport>");
		MS_DEBUG_DEV("  ssrc          : %" PRIu32, (uint32_t)ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  fraction lost : %" PRIu32, this->header->fraction_lost);
		MS_DEBUG_DEV("  total lost    : %" PRIu32, this->GetTotalLost());
		MS_DEBUG_DEV("  last seq      : %" PRIu32, (uint32_t)ntohl(this->header->last_seq));
		MS_DEBUG_DEV("  jitter        : %" PRIu32, (uint32_t)ntohl(this->header->jitter));
		MS_DEBUG_DEV("  lsr           : %" PRIu32, (uint32_t)ntohl(this->header->lsr));
		MS_DEBUG_DEV("  dlsr          : %" PRIu32, (uint32_t)ntohl(this->header->dlsr));
		MS_DEBUG_DEV("</ReceiverReport>");
	}

	void ReceiverReport::Serialize()
	{
		MS_TRACE();

		// Allocate internal data.
		if (!this->raw)
		{
			this->raw = new uint8_t[sizeof(Header)];

			// Copy the header.
			std::memcpy(this->raw, this->header, sizeof(Header));

			// Update the header pointer.
			this->header = reinterpret_cast<Header*>(this->raw);
		}
	}

	size_t ReceiverReport::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// Copy the header.
		std::memcpy(buffer, this->header, sizeof(Header));

		return sizeof(Header);
	}

	/* Class methods. */

	/**
	 * ReceiverReportPacket::Parse()
	 * @param  data   - Points to the begining of the incoming RTCP packet.
	 * @param  len    - Total length of the packet.
	 * @param  offset - points to the first Receiver Report if the incoming packet.
	 */
	ReceiverReportPacket* ReceiverReportPacket::Parse(const uint8_t* data, size_t len, size_t offset)
	{
		MS_TRACE();

		// Get the header.
		Packet::CommonHeader* header = (Packet::CommonHeader*)data;
		std::unique_ptr<ReceiverReportPacket> packet(new ReceiverReportPacket());

		packet->SetSsrc(Utils::Byte::Get4Bytes((uint8_t*)header, sizeof(CommonHeader)));

		if (offset == 0)
			offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

		uint8_t count = header->count;
		while ((count--) && (len - offset > 0))
		{
			ReceiverReport* report = ReceiverReport::Parse(data+offset, len-offset);
			if (report)
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

	size_t ReceiverReportPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		size_t offset = Packet::Serialize(buffer);

		// Copy the SSRC.
		std::memcpy(buffer + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
		offset += sizeof(this->ssrc);

		// Serialize reports.
		for (auto report : this->reports)
		{
			offset += report->Serialize(buffer + offset);
		}

		return offset;
	}

	void ReceiverReportPacket::Dump()
	{
		MS_TRACE();

		#ifdef MS_LOG_DEV

		MS_DEBUG_DEV("<ReceiverReportPacket>");
		MS_DEBUG_DEV("  ssrc: %" PRIu32, (uint32_t)ntohl(this->ssrc));
		for (auto report : this->reports)
		{
			report->Dump();
		}
		MS_DEBUG_DEV("</ReceiverReportPacket>");

		#endif
	}
}}
