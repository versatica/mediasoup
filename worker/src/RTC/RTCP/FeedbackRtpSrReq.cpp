#define MS_CLASS "RTC::RTCP::FeedbackRtpSrReqPacket"

#include "RTC/RTCP/FeedbackRtpSrReq.h"
#include "Logger.h"

namespace RTC { namespace RTCP
{
	/* Class methods. */

	FeedbackRtpSrReqPacket* FeedbackRtpSrReqPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
			MS_WARN("not enough space for Feedback packet, discarded");
			return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;

		return new FeedbackRtpSrReqPacket(commonHeader);
	}

	void FeedbackRtpSrReqPacket::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\t<FeedbackRtpSrReqPacket>");
		FeedbackRtpPacket::Dump();
		MS_WARN("\t</FeedbackRtpSrReqPacket>");
	}
}}
