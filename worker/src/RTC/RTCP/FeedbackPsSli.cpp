#define MS_CLASS "RTC::RTCP::FeedbackPsSliPacket"
// #define MS_LOG_DEV

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
			MS_WARN_TAG(rtcp, "not enough space for Sli item, discarded");

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

		uint32_t compact = (uint32_t)ntohl(header->compact);

		this->first = compact >> 19;            /* first 13 bits */
		this->number = (compact >> 6) & 0x1fff; /* next  13 bits */
		this->pictureId = compact & 0x3f;       /* last   6 bits */
	}

	size_t SliItem::Serialize(uint8_t* buffer)
	{
		uint32_t compact = (this->first << 19) | (this->number << 6) | this->pictureId;
		Header* header = (Header*)buffer;

		header->compact = htonl(compact);
		std::memcpy(buffer, header, sizeof(Header));

		return sizeof(Header);
	}

	void SliItem::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<SliItem>");
		MS_DEBUG_DEV("  first      : %" PRIu16, this->first);
		MS_DEBUG_DEV("  number     : %" PRIu16, this->number);
		MS_DEBUG_DEV("  picture id : %" PRIu8, this->pictureId);
		MS_DEBUG_DEV("</SliItem>");
	}
}}

