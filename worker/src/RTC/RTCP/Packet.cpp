#define MS_CLASS "RTC::Packet"

#include "RTC/RTCP/Packet.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/Sdes.h"
#include "RTC/RTCP/Feedback.h"
#include "Logger.h"

namespace RTC
{
namespace RTCP
{
	/* Namespace methods */
	const std::string& Type2String(Type type)
	{
		static const std::string unknown("UNKNOWN");

		if (type2String.find(type) == type2String.end())
			return unknown;

		return type2String[type];
	}

	/* Class methods. */

	Packet* Packet::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// First, Currently parsing and Last RTCP packets in the compound packet
		Packet *first, *current, *last;
		first = current = last = nullptr;

		while (int(len) > 0)
		{
			if (!Packet::IsRtcp(data, len))
			{
				MS_WARN("data is not a RTCP packet");

				return first;
			}

			CommonHeader* header = (CommonHeader*)data;
			size_t packet_len = (size_t)(ntohs(header->length) + 1) * 4;

			if (len < packet_len)
			{
				MS_WARN("packet length exceeds remaining data [len:%zu, packet_len:%zu]", len, packet_len);

				return first;
			}

			switch (Type(header->packet_type))
			{
				case Type::SR:
				{
					current = SenderReportPacket::Parse(data, len);

					if (!current) {
						break;
					}

					if ((uint8_t)header->count > 0) {
						Packet* rr = ReceiverReportPacket::Parse(data, len, current->GetSize());

						if (!rr) {
							break;
						}

						current->SetNext(rr);
					}
				}
					break;

				case Type::RR:
					current = ReceiverReportPacket::Parse(data, len);
					break;

				case Type::SDES:
					current = SdesPacket::Parse(data, len);
					break;

				case Type::BYE:
					current = nullptr;
					break;

				case Type::APP:
					current = nullptr;
					break;

				case Type::PSFB:
					current = FeedbackPsPacket::Parse(data, len);
					current->Dump();
					break;

				case Type::RTPFB:
					current = FeedbackRtpPacket::Parse(data, len);
					current->Dump();
					break;

				default:
					MS_WARN("unknown RTCP packet type [packet_type:%" PRIu8 "]", header->packet_type);
					current = nullptr;
			}

			if (!current) {
				MS_WARN("error parsing %s Packet", Type2String(Type(header->packet_type)).c_str());
				return first;
			}

			//current->Dump();

			data += packet_len;
			len -= packet_len;

			if (!first)
				first = current;
			else
				last->SetNext(current);

			last = current->GetNext() ? current->GetNext() : current;

		}

		return first;
	}

	/* Instance methods. */

	Packet::Packet(Type type)
		:type(type)
	{
	}

	Packet::~Packet()
	{
		if (this->raw)
			delete raw;
	}

	size_t Packet::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t length = (this->GetSize() / 4) - 1;

		CommonHeader* header = (Packet::CommonHeader* )data;

		// Fill the common header
		header->version = 2;
		header->padding = 0;
		header->count = (uint8_t)this->GetCount();
		header->packet_type = (uint8_t) this->type;
		header->length = htons(length);

		return sizeof(CommonHeader);
	}
}
}
