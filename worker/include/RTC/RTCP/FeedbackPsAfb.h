#ifndef MS_RTC_RTCP_FEEDBACK_AFB_H
#define MS_RTC_RTCP_FEEDBACK_AFB_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

namespace RTC { namespace RTCP
{

	class FeedbackPsAfbPacket
		: public FeedbackPsPacket
	{

	public:
		static FeedbackPsAfbPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsAfbPacket(CommonHeader* commonHeader);
		FeedbackPsAfbPacket(uint32_t sender_ssrc, uint32_t media_ssrc);

		size_t GetSize() override;
		size_t Serialize(uint8_t* data) override;
		void Dump() override;

	private:
		uint8_t *data;
		size_t size;
	};

	/* inline instance methods. */

	inline
	FeedbackPsAfbPacket::FeedbackPsAfbPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
		this->size = ((ntohs(commonHeader->length) + 1) * 4) - (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header));

		this->data = (uint8_t*)commonHeader + sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);
	}

	inline
	FeedbackPsAfbPacket::FeedbackPsAfbPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::AFB, sender_ssrc, media_ssrc)
	{
	}

	inline
	size_t FeedbackPsAfbPacket::GetSize()
	{
		return FeedbackPsPacket::GetSize() + this->size;
	}

} } // RTP::RTCP

#endif
