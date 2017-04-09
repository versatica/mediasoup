#ifndef MS_RTC_RTCP_FEEDBACK_PS_LEI_HPP
#define MS_RTC_RTCP_FEEDBACK_PS_LEI_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"

/* RFC 6642
 * Payload-Specific Third-Party Loss Early Indication (PSLEI)
 *
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
0   |                              SSRC                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsLeiItem : public FeedbackItem
		{
		private:
			struct Header
			{
				uint32_t ssrc;
			};

		public:
			static const FeedbackPs::MessageType MessageType = FeedbackPs::MessageType::PSLEI;

		public:
			static FeedbackPsLeiItem* Parse(const uint8_t* data, size_t len);

		public:
			explicit FeedbackPsLeiItem(Header* header);
			explicit FeedbackPsLeiItem(FeedbackPsLeiItem* item);
			explicit FeedbackPsLeiItem(uint32_t ssrc);
			virtual ~FeedbackPsLeiItem(){};

			uint32_t GetSsrc() const;

			/* Virtual methods inherited from FeedbackItem. */
		public:
			virtual void Dump() const override;
			virtual size_t Serialize(uint8_t* buffer) override;
			virtual size_t GetSize() const override;

		private:
			Header* header = nullptr;
		};

		// Lei packet declaration.
		typedef FeedbackPsItemsPacket<FeedbackPsLeiItem> FeedbackPsLeiPacket;

		/* Inline instance methods. */

		inline FeedbackPsLeiItem::FeedbackPsLeiItem(Header* header)
		    : header(header)
		{
		}

		inline FeedbackPsLeiItem::FeedbackPsLeiItem(FeedbackPsLeiItem* item)
		    : header(item->header)
		{
		}

		inline size_t FeedbackPsLeiItem::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint32_t FeedbackPsLeiItem::GetSsrc() const
		{
			return (uint32_t)ntohl(this->header->ssrc);
		}
	}
}

#endif
