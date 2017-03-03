#define MS_CLASS "RTC::RTCP::CompoundPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/CompoundPacket.hpp"
#include "Logger.hpp"

namespace RTC { namespace RTCP
{
	/* Instance methods. */

	void CompoundPacket::Serialize(uint8_t* data)
	{
		MS_TRACE();

		this->header = data;

		// Calculate the total required size for the entire message.
		if (this->senderReportPacket.GetCount())
		{
			this->size = this->senderReportPacket.GetSize();

			if (this->receiverReportPacket.GetCount())
				this->size += sizeof(ReceiverReport::Header) * this->receiverReportPacket.GetCount();
		}
		// If no sender nor receiver reports are present send an empty Receiver Report
		// packet as the head of the compound packet.
		else
		{
			this->size = this->receiverReportPacket.GetSize();
		}

		if (this->sdesPacket.GetCount())
			this->size += this->sdesPacket.GetSize();

		// Fill it.
		size_t offset = 0;

		if (this->senderReportPacket.GetCount())
		{
			this->senderReportPacket.Serialize(this->header);
			offset = this->senderReportPacket.GetSize();

			if (this->receiverReportPacket.GetCount())
			{
				// Fix header length field.
				Packet::CommonHeader* header = (Packet::CommonHeader*)this->header;
				size_t length = ((sizeof(SenderReport::Header) + (sizeof(ReceiverReport::Header) * this->receiverReportPacket.GetCount())) / 4);

				header->length = htons(length);

				// Fix header count field.
				header->count = this->receiverReportPacket.GetCount();

				ReceiverReportPacket::Iterator it = this->receiverReportPacket.Begin();
				for (; it != this->receiverReportPacket.End(); ++it)
				{
					ReceiverReport* report = (*it);

					report->Serialize(this->header + offset);
					offset += sizeof(ReceiverReport::Header);
				}
			}
		}
		else
		{
			this->receiverReportPacket.Serialize(this->header);
			offset = this->receiverReportPacket.GetSize();
		}

		if (this->sdesPacket.GetCount())
			this->sdesPacket.Serialize(this->header + offset);
	}

	void CompoundPacket::Dump()
	{
		MS_TRACE();

		MS_DUMP("<CompoundPacket>");

		if (this->senderReportPacket.GetCount())
		{
			this->senderReportPacket.Dump();

			if (this->receiverReportPacket.GetCount())
			{
				ReceiverReportPacket::Iterator it = this->receiverReportPacket.Begin();

				for (; it != this->receiverReportPacket.End(); ++it)
				{
					(*it)->Dump();
				}
			}
		}
		else
		{
			this->receiverReportPacket.Dump();
		}

		if (this->sdesPacket.GetCount())
			this->sdesPacket.Dump();

		MS_DUMP("</CompoundPacket>");
	}

	void CompoundPacket::AddSenderReport(SenderReport* report)
	{
		MS_ASSERT(this->senderReportPacket.GetCount() == 0, "a sender report is already present");

		this->senderReportPacket.AddReport(report);
	}
}}
