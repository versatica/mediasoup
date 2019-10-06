#define MS_CLASS "RTC::RTCP::XrReceiverReferenceTime"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		ReceiverReferenceTime* ReceiverReferenceTime::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Ensure there is space for the common header and the body.
			if (len < sizeof(ExtendedReportBlock::CommonHeader) + sizeof(ReceiverReferenceTime::Body))
			{
				MS_WARN_TAG(rtcp, "not enough space for a extended RRT block, block discarded");

				return nullptr;
			}

			// Get the header.
			auto* header = const_cast<ExtendedReportBlock::CommonHeader*>(
			  reinterpret_cast<const ExtendedReportBlock::CommonHeader*>(data));

			return new ReceiverReferenceTime(header);
		}

		void ReceiverReferenceTime::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ReceiverReferenceTime>");
			MS_DUMP("  block type : %" PRIu8, static_cast<uint8_t>(this->type));
			MS_DUMP("  reserved   : 0");
			MS_DUMP("  length     : 2");
			MS_DUMP("  ntp sec    : %" PRIu32, GetNtpSec());
			MS_DUMP("  ntp frac   : %" PRIu32, GetNtpFrac());
			MS_DUMP("</ReceiverReferenceTime>");
		}

		size_t ReceiverReferenceTime::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Fill the common header.
			this->header->blockType = static_cast<uint8_t>(this->type);
			this->header->reserved  = 0;
			this->header->length    = htons(2);

			std::memcpy(buffer, this->header, sizeof(ExtendedReportBlock::CommonHeader));

			size_t offset{ sizeof(ExtendedReportBlock::CommonHeader) };

			// Copy the body.
			std::memcpy(buffer + offset, this->body, sizeof(ReceiverReferenceTime::Body));

			offset += sizeof(ReceiverReferenceTime::Body);

			return offset;
		}
	} // namespace RTCP
} // namespace RTC
