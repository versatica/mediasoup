#define MS_CLASS "RTC::RTCP::FeedbackRtpSrReq"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackRtpSrReq.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
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

			auto* commonHeader = reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(data));

			return new FeedbackRtpSrReqPacket(commonHeader);
		}

		void FeedbackRtpSrReqPacket::Dump() const
		{
			MS_TRACE();

			MS_DEBUG_DEV("<FeedbackRtpSrReqPacket>");
			FeedbackRtpPacket::Dump();
			MS_DEBUG_DEV("</FeedbackRtpSrReqPacket>");
		}
	} // namespace RTCP
} // namespace RTC
