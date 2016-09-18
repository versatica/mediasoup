#define MS_CLASS "RTC::RtcpPacket"

#include "RTC/RtcpPacket.h"
#include "Logger.h"

namespace RTC
{
	/* Class methods. */

	RtcpPacket* RtcpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RtcpPacket::IsRtcp(data, len))
			return nullptr;

		// TODO: just for now
		CommonHeader* header = (CommonHeader*)data;
		RtcpPacket* compoundPacket = new RtcpPacket(header, data, len);

		// MS_DEBUG("RTCP compound packet parsing begins");

		bool more = true;

		while (more)
		{
			if (!RtcpPacket::IsRtcp(data, len))
			{
				MS_WARN("data is not a RTCP packet");

				return nullptr;
			}

			CommonHeader* header = (CommonHeader*)data;
			size_t packet_len = (size_t)(ntohs(header->length) + 1) * 4;

			if (len < packet_len)
			{
				MS_WARN("packet length exceeds remaining data [len:%zu, packet_len:%zu]", len, packet_len);

				return nullptr;
			}

			// TODO: test
			// switch (header->packet_type)
			// {
			// 	case 200:
			// 		MS_DEBUG("SENDER REPORT packet found");
			// 		break;
			// 	case 201:
			// 		MS_DEBUG("RECEIVER REPORT packet found");
			// 		break;
			// 	case 202:
			// 		MS_DEBUG("SDES packet found");
			// 		break;
			// 	case 203:
			// 		MS_DEBUG("BYE packet found");
			// 		break;
			// 	case 204:
			// 		MS_DEBUG("APPLICATION DEFINED packet found");
			// 		break;
			// 	default:
			// 		MS_WARN("unknown RTCP packet type [packet_type:%" PRIu8 "]", header->packet_type);
			// }

			data += packet_len;
			len -= packet_len;

			if (len == 0)
				more = false;
		}

		// MS_DEBUG("RTCP compound packet parsing done");

		return compoundPacket;
	}

	/* Instance methods. */

	RtcpPacket::RtcpPacket(CommonHeader* header, const uint8_t* raw, size_t length) :
		header(header),
		raw((uint8_t*)raw),
		length(length)
	{
		MS_TRACE();
	}

	RtcpPacket::~RtcpPacket()
	{
		MS_TRACE();
	}
}
