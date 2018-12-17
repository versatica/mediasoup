#define MS_CLASS "RTC::RTCP::FeedbackPsSli"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsSli.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Instance methods. */

		FeedbackPsSliItem::FeedbackPsSliItem(Header* header)
		{
			MS_TRACE();

			this->header = header;

			auto compact = uint32_t{ ntohl(header->compact) };

			this->first     = compact >> 19;           /* first 13 bits */
			this->number    = (compact >> 6) & 0x1fff; /* next  13 bits */
			this->pictureId = compact & 0x3f;          /* last   6 bits */
		}

		size_t FeedbackPsSliItem::Serialize(uint8_t* buffer)
		{
			uint32_t compact = (this->first << 19) | (this->number << 6) | this->pictureId;
			auto* header     = reinterpret_cast<Header*>(buffer);

			header->compact = uint32_t{ htonl(compact) };
			std::memcpy(buffer, header, sizeof(Header));

			return sizeof(Header);
		}

		void FeedbackPsSliItem::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<FeedbackPsSliItem>");
			MS_DEBUG_DEV("  first      : %" PRIu16, this->first);
			MS_DEBUG_DEV("  number     : %" PRIu16, this->number);
			MS_DEBUG_DEV("  picture id : %" PRIu8, this->pictureId);
			MS_DEBUG_DEV("</FeedbackPsSliItem>");
		}
	} // namespace RTCP
} // namespace RTC
