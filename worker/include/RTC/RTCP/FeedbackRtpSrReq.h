#ifndef MS_RTC_RTCP_FEEDBACK_SR_REQ_H
#define MS_RTC_RTCP_FEEDBACK_SR_REQ_H

#include "common.h"
#include "RTC/RTCP/Feedback.h"

namespace RTC { namespace RTCP
{

	class FeedbackRtpSrReqPacket
		: public FeedbackRtpPacket
	{

	public:
		static FeedbackRtpSrReqPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackRtpSrReqPacket(CommonHeader* commonHeader);
		FeedbackRtpSrReqPacket(uint32_t sender_ssrc, uint32_t media_ssrc);

		void Dump() override;
	};

	/* inline instance methods. */

	inline
	FeedbackRtpSrReqPacket::FeedbackRtpSrReqPacket(CommonHeader* commonHeader):
		FeedbackRtpPacket(commonHeader)
	{
	}

	inline
	FeedbackRtpSrReqPacket::FeedbackRtpSrReqPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackRtpPacket(FeedbackRtp::SR_REQ, sender_ssrc, media_ssrc)
	{
	}

} } // RTP::RTCP

#endif
