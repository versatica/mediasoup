#define MS_CLASS "RTC::RTCP::Packet"
// #define MS_LOG_DEV

#include "RTC/RTCP/Packet.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/Sdes.h"
#include "RTC/RTCP/Bye.h"
#include "RTC/RTCP/Feedback.h"
#include "Logger.h"

namespace RTC { namespace RTCP
{
	/* Class variables. */

	std::map<Type, std::string> Packet::type2String =
	{
		{ Type::FIR,   "FIR"   },
		{ Type::NACK,  "NACK"  },
		{ Type::SR,    "SR"    },
		{ Type::RR,    "RR"    },
		{ Type::SDES,  "SDES"  },
		{ Type::BYE,   "BYE"   },
		{ Type::APP,   "APP"   },
		{ Type::RTPFB, "RTPFB" },
		{ Type::PSFB,  "PSFB"  }
	};

	/* Class methods. */

	Packet* Packet::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// First, Currently parsing and Last RTCP packets in the compound packet.
		Packet *first, *current, *last;
		first = current = last = nullptr;

		while (int(len) > 0)
		{
			if (!Packet::IsRtcp(data, len))
			{
				MS_WARN_TAG(rtcp, "data is not a RTCP packet");

				return first;
			}

			CommonHeader* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
			size_t packet_len = (size_t)(ntohs(header->length) + 1) * 4;

			if (len < packet_len)
			{
				MS_WARN_TAG(rtcp, "packet length exceeds remaining data [len:%zu, packet_len:%zu]", len, packet_len);

				return first;
			}

			switch (Type(header->packet_type))
			{
				case Type::SR:
				{
					current = SenderReportPacket::Parse(data, len);

					if (!current)
						break;

					if ((uint8_t)header->count > 0)
					{
						Packet* rr = ReceiverReportPacket::Parse(data, len, current->GetSize());
						if (!rr)
							break;

						current->SetNext(rr);
					}

					break;
				}

				case Type::RR:
				{
					current = ReceiverReportPacket::Parse(data, len);
					break;
				}

				case Type::SDES:
				{
					current = SdesPacket::Parse(data, len);
					break;
				}

				case Type::BYE:
				{
					current = ByePacket::Parse(data, len);
					break;
				}

				case Type::APP:
				{
					current = nullptr;
					break;
				}

				case Type::PSFB:
				{
					current = FeedbackPsPacket::Parse(data, len);
					break;
				}

				case Type::RTPFB:
				{
					current = FeedbackRtpPacket::Parse(data, len);
					break;
				}

				default:
				{
					MS_WARN_TAG(rtcp, "unknown RTCP packet type [packet_type:%" PRIu8 "]", header->packet_type);

					current = nullptr;
				}
			}

			if (!current)
			{
				std::string packetType = Type2String(Type(header->packet_type));

				if (Type(header->packet_type) == Type::PSFB)
					packetType += " " + FeedbackPsPacket::MessageType2String(FeedbackPs::MessageType(header->count));
				else if (Type(header->packet_type) == Type::RTPFB)
					packetType += " " + FeedbackRtpPacket::MessageType2String(FeedbackRtp::MessageType(header->count));

				MS_WARN_TAG(rtcp, "error parsing %s Packet", packetType.c_str());

				return first;
			}

			#ifdef MS_LOG_DEV
			// current->Dump();
			#endif

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

	const std::string& Packet::Type2String(Type type)
	{
		static const std::string unknown("UNKNOWN");

		if (Packet::type2String.find(type) == Packet::type2String.end())
			return unknown;

		return Packet::type2String[type];
	}

	/* Instance methods. */

	size_t Packet::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		this->header = reinterpret_cast<CommonHeader*>(buffer);

		size_t length = (this->GetSize() / 4) - 1;

		// Fill the common header.
		this->header->version = 2;
		this->header->padding = 0;
		this->header->count = (uint8_t)this->GetCount();
		this->header->packet_type = (uint8_t)this->type;
		this->header->length = htons(length);

		return sizeof(CommonHeader);
	}
}}
