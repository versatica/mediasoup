#define MS_CLASS "RTC::RTCP::FeedbackPsSli"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/FeedbackPsSli.hpp"
#include "Logger.hpp"

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
			const uint32_t compact = (this->first << 19) | (this->number << 6) | this->pictureId;
			auto* header           = reinterpret_cast<Header*>(buffer);

			header->compact = uint32_t{ htonl(compact) };
			std::memcpy(buffer, header, HeaderSize);

			return HeaderSize;
		}

		void FeedbackPsSliItem::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<FeedbackPsSliItem>");
			MS_DUMP_CLEAN(indentation, "  first: %" PRIu16, this->first);
			MS_DUMP_CLEAN(indentation, "  number: %" PRIu16, this->number);
			MS_DUMP_CLEAN(indentation, "  picture id: %" PRIu8, this->pictureId);
			MS_DUMP_CLEAN(indentation, "</FeedbackPsSliItem>");
		}
	} // namespace RTCP
} // namespace RTC
