#define MS_CLASS "RTC::RTCP::FeedbackPsFir"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "Logger.hpp"
#include <cstring>
#include <cstring> // std::memset()

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		FeedbackPsFirItem* FeedbackPsFirItem::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// data size must be >= header.
			if (sizeof(Header) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for Fir item, discarded");

				return nullptr;
			}

			auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

			return new FeedbackPsFirItem(header);
		}

		/* Instance methods. */

		FeedbackPsFirItem::FeedbackPsFirItem(uint32_t ssrc, uint8_t sequenceNumber)
		{
			MS_TRACE();

			this->raw    = new uint8_t[sizeof(Header)];
			this->header = reinterpret_cast<Header*>(this->raw);

			// Set reserved bits to zero.
			std::memset(this->header, 0, sizeof(Header));

			this->header->ssrc           = uint32_t{ htonl(ssrc) };
			this->header->sequenceNumber = sequenceNumber;
		}

		size_t FeedbackPsFirItem::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			std::memcpy(buffer, this->header, sizeof(Header));

			return sizeof(Header);
		}

		void FeedbackPsFirItem::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<FeedbackPsFirItem>");
			MS_DUMP("  ssrc            : %" PRIu32, this->GetSsrc());
			MS_DUMP("  sequence number : %" PRIu8, this->GetSequenceNumber());
			MS_DUMP("</FeedbackPsFirItem>");
		}
	} // namespace RTCP
} // namespace RTC
