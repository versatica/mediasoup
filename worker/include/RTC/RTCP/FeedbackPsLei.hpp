#ifndef MS_RTC_RTCP_FEEDBACK_PSLEI_HPP
#define MS_RTC_RTCP_FEEDBACK_PSLEI_HPP

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

namespace RTC { namespace RTCP
{
	class PsLeiItem
		: public FeedbackItem
	{
	private:
		struct Header
		{
			uint32_t ssrc;
		};

	public:
		static const FeedbackPs::MessageType MessageType = FeedbackPs::PSLEI;

	public:
		static PsLeiItem* Parse(const uint8_t* data, size_t len);

	public:
		explicit PsLeiItem(Header* header);
		explicit PsLeiItem(PsLeiItem* item);
		explicit PsLeiItem(uint32_t ssrc);
		virtual ~PsLeiItem() {};

		uint32_t GetSsrc() const;

	/* Virtual methods inherited from FeedbackItem. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		Header* header = nullptr;
	};

	// Lei packet declaration.
	typedef FeedbackPsItemPacket<PsLeiItem> FeedbackPsLeiPacket;

	/* Inline instance methods. */

	inline
	PsLeiItem::PsLeiItem(Header* header):
		header(header)
	{}

	inline
	PsLeiItem::PsLeiItem(PsLeiItem* item):
		header(item->header)
	{}

	inline
	size_t PsLeiItem::GetSize() const
	{
		return sizeof(Header);
	}

	inline
	uint32_t PsLeiItem::GetSsrc() const
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}
}}

#endif
