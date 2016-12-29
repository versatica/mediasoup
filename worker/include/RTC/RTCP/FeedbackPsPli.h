#ifndef MS_RTC_RTCP_FEEDBACK_PLI_H
#define MS_RTC_RTCP_FEEDBACK_PLI_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

namespace RTC { namespace RTCP
{

	class FeedbackPsPliPacket
		: public FeedbackPsPacket
	{

	public:
		static FeedbackPsPliPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackPsPliPacket(CommonHeader* commonHeader);
		FeedbackPsPliPacket(uint32_t sender_ssrc, uint32_t media_ssrc);

		void Dump() override;
	};

	/* inline instance methods. */

	inline
	FeedbackPsPliPacket::FeedbackPsPliPacket(CommonHeader* commonHeader):
		FeedbackPsPacket(commonHeader)
	{
	}

	inline
	FeedbackPsPliPacket::FeedbackPsPliPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsPacket(FeedbackPs::PLI, sender_ssrc, media_ssrc)
	{
	}

} } // RTP::RTCP

#endif
