#define MS_CLASS "RTC::RTCP::FeedbackRtpSrReqPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpSrReq.hpp"
#include "Logger.hpp"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackRtpSrReqPacket* FeedbackRtpSrReqPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;

		return new FeedbackRtpSrReqPacket(commonHeader);
	}

	void FeedbackRtpSrReqPacket::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<FeedbackRtpSrReqPacket>");
		FeedbackRtpPacket::Dump();
		MS_DEBUG_DEV("</FeedbackRtpSrReqPacket>");

		#endif
	}
}}
