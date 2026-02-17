#include "RTC/RTCP/FuzzerFeedbackPs.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsAfb.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsFir.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsLei.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsPli.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsRemb.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsRpsi.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsSli.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsTst.hpp"
#include "RTC/RTCP/FuzzerFeedbackPsVbcm.hpp"

void FuzzerRtcRtcpFeedbackPs::Fuzz(RTC::RTCP::Packet* packet)
{
	auto* fbps = dynamic_cast<RTC::RTCP::FeedbackPsPacket*>(packet);

	fbps->GetMessageType();
	fbps->GetSenderSsrc();
	fbps->SetSenderSsrc(1111);
	fbps->GetMediaSsrc();
	fbps->SetMediaSsrc(2222);

	switch (fbps->GetMessageType())
	{
		case RTC::RTCP::FeedbackPs::MessageType::PLI:
		{
			auto* pli = dynamic_cast<RTC::RTCP::FeedbackPsPliPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsPli::Fuzz(pli);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::SLI:
		{
			auto* sli = dynamic_cast<RTC::RTCP::FeedbackPsSliPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsSli::Fuzz(sli);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::RPSI:
		{
			auto* rpsi = dynamic_cast<RTC::RTCP::FeedbackPsRpsiPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsRpsi::Fuzz(rpsi);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::FIR:
		{
			auto* fir = dynamic_cast<RTC::RTCP::FeedbackPsFirPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsFir::Fuzz(fir);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::TSTR:
		{
			auto* tstr = dynamic_cast<RTC::RTCP::FeedbackPsTstrPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsTstr::Fuzz(tstr);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::TSTN:
		{
			auto* tstn = dynamic_cast<RTC::RTCP::FeedbackPsTstnPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsTstn::Fuzz(tstn);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::VBCM:
		{
			auto* vbcm = dynamic_cast<RTC::RTCP::FeedbackPsVbcmPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsVbcm::Fuzz(vbcm);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::PSLEI:
		{
			auto* lei = dynamic_cast<RTC::RTCP::FeedbackPsLeiPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsLei::Fuzz(lei);

			break;
		}

		case RTC::RTCP::FeedbackPs::MessageType::AFB:
		{
			auto* afb = dynamic_cast<RTC::RTCP::FeedbackPsAfbPacket*>(fbps);

			FuzzerRtcRtcpFeedbackPsAfb::Fuzz(afb);

			break;
		}

		default:
		{
		}
	}
}
