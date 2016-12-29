#define MS_CLASS "RTC::RTCP::FeedbackPsSliPacket"

#include "RTC/RTCP/FeedbackPsSli.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	SliItem* SliItem::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN("not enough space for Sli item, discarded");
			return nullptr;
		}

		Header* header = (Header*)data;

		return new SliItem(header);
	}

	/* Instance methods. */

	SliItem::SliItem(Header* header)
	{
		MS_TRACE();

		this->header = header;

		uint32_t compact = ntohl(header->compact);

		this->first = compact >> 19;            /* first 13 bits */
		this->number = (compact >> 6) & 0x1fff; /* next  13 bits */
		this->pictureId = compact & 0x3f;       /* last   6 bits */
	}

	size_t SliItem::Serialize(uint8_t* data)
	{
		uint32_t compact = (this->first << 19) | (this->number << 6) | this->pictureId;
		Header* header = (Header*)data;

		header->compact = htonl(compact);
		std::memcpy(data, header, sizeof(Header));

		return sizeof(Header);
	}

	void SliItem::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t\t<Sli Item>");
		MS_WARN("\t\t\tfirst: %u", this->first);
		MS_WARN("\t\t\tnumber: %u", this->number);
		MS_WARN("\t\t\tpictureId: %u", this->pictureId);
		MS_WARN("\t\t</Sli Item>");
	}
}}

