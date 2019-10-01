#include "RTC/RTCP/FuzzerFeedbackRtp.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpEcn.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpNack.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpSrReq.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTllei.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTmmb.hpp"
#include "RTC/RTCP/FuzzerFeedbackRtpTransport.hpp"

void Fuzzer::RTC::RTCP::FeedbackRtp::Fuzz(::RTC::RTCP::Packet* packet)
{
	auto* fbrtp = dynamic_cast<::RTC::RTCP::FeedbackRtpPacket*>(packet);

	fbrtp->GetMessageType();
	fbrtp->GetSenderSsrc();
	fbrtp->SetSenderSsrc(1111);
	fbrtp->GetMediaSsrc();
	fbrtp->SetMediaSsrc(2222);

	switch (fbrtp->GetMessageType())
	{
		case ::RTC::RTCP::FeedbackRtp::MessageType::NACK:
		{
			auto* nack = dynamic_cast<::RTC::RTCP::FeedbackRtpNackPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpNack::Fuzz(nack);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::TMMBR:
		{
			auto* tmmbr = dynamic_cast<::RTC::RTCP::FeedbackRtpTmmbrPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpTmmbr::Fuzz(tmmbr);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::TMMBN:
		{
			auto* tmmbn = dynamic_cast<::RTC::RTCP::FeedbackRtpTmmbnPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpTmmbn::Fuzz(tmmbn);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::SR_REQ:
		{
			auto* srReq = dynamic_cast<::RTC::RTCP::FeedbackRtpSrReqPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpSrReq::Fuzz(srReq);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::TLLEI:
		{
			auto* tllei = dynamic_cast<::RTC::RTCP::FeedbackRtpTlleiPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpTllei::Fuzz(tllei);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::ECN:
		{
			auto* ecn = dynamic_cast<::RTC::RTCP::FeedbackRtpEcnPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpEcn::Fuzz(ecn);

			break;
		}

		case ::RTC::RTCP::FeedbackRtp::MessageType::TCC:
		{
			auto* feedback = dynamic_cast<::RTC::RTCP::FeedbackRtpTransportPacket*>(fbrtp);

			Fuzzer::RTC::RTCP::FeedbackRtpTransport::Fuzz(feedback);

			break;
		}

		default:
		{
		}
	}
}
