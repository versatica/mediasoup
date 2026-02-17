#include "RTC/RTCP/FuzzerFeedbackRtp.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpEcn.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpNack.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpSrReq.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTllei.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTmmb.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

void FuzzerRtcRtcpFeedbackRtp::Fuzz(RTC::RTCP::Packet* packet)
{
	auto* fbrtp = dynamic_cast<RTC::RTCP::FeedbackRtpPacket*>(packet);

	fbrtp->GetMessageType();
	fbrtp->GetSenderSsrc();
	fbrtp->SetSenderSsrc(1111);
	fbrtp->GetMediaSsrc();
	fbrtp->SetMediaSsrc(2222);

	switch (fbrtp->GetMessageType())
	{
		case RTC::RTCP::FeedbackRtp::MessageType::NACK:
		{
			auto* nack = dynamic_cast<RTC::RTCP::FeedbackRtpNackPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpNack::Fuzz(nack);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::TMMBR:
		{
			auto* tmmbr = dynamic_cast<RTC::RTCP::FeedbackRtpTmmbrPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpTmmbr::Fuzz(tmmbr);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::TMMBN:
		{
			auto* tmmbn = dynamic_cast<RTC::RTCP::FeedbackRtpTmmbnPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpTmmbn::Fuzz(tmmbn);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::SR_REQ:
		{
			auto* srReq = dynamic_cast<RTC::RTCP::FeedbackRtpSrReqPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpSrReq::Fuzz(srReq);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::TLLEI:
		{
			auto* tllei = dynamic_cast<RTC::RTCP::FeedbackRtpTlleiPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpTllei::Fuzz(tllei);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::ECN:
		{
			auto* ecn = dynamic_cast<RTC::RTCP::FeedbackRtpEcnPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpEcn::Fuzz(ecn);

			break;
		}

		case RTC::RTCP::FeedbackRtp::MessageType::TCC:
		{
			auto* feedback = dynamic_cast<RTC::RTCP::FeedbackRtpTransportPacket*>(fbrtp);

			FuzzerRtcRtcpFeedbackRtpTransport::Fuzz(feedback);

			break;
		}

		default:
		{
		}
	}
}
