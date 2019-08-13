#ifndef MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TRANSPORT
#define MS_FUZZER_RTC_RTCP_FEEDBACK_RTP_TRANSPORT

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTCP
		{
			namespace FeedbackRtpTransport
			{
				void Fuzz(::RTC::RTCP::FeedbackRtpTransportPacket* packet);
			}
		} // namespace RTCP
	}   // namespace RTC
} // namespace Fuzzer

#endif
