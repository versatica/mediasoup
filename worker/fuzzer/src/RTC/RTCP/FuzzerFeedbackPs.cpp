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

void Fuzzer::RTC::RTCP::FeedbackPs::Fuzz(::RTC::RTCP::Packet* packet)
{
	auto* fbps = dynamic_cast<::RTC::RTCP::FeedbackPsPacket*>(packet);

	fbps->GetMessageType();
	fbps->GetSenderSsrc();
	fbps->SetSenderSsrc(1111);
	fbps->GetMediaSsrc();
	fbps->SetMediaSsrc(2222);

	switch (fbps->GetMessageType())
	{
		case ::RTC::RTCP::FeedbackPs::MessageType::PLI:
		{
			auto* pli = dynamic_cast<::RTC::RTCP::FeedbackPsPliPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsPli::Fuzz(pli);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::SLI:
		{
			auto* sli = dynamic_cast<::RTC::RTCP::FeedbackPsSliPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsSli::Fuzz(sli);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::RPSI:
		{
			auto* rpsi = dynamic_cast<::RTC::RTCP::FeedbackPsRpsiPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsRpsi::Fuzz(rpsi);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::FIR:
		{
			auto* fir = dynamic_cast<::RTC::RTCP::FeedbackPsFirPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsFir::Fuzz(fir);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::TSTR:
		{
			auto* tstr = dynamic_cast<::RTC::RTCP::FeedbackPsTstrPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsTstr::Fuzz(tstr);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::TSTN:
		{
			auto* tstn = dynamic_cast<::RTC::RTCP::FeedbackPsTstnPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsTstn::Fuzz(tstn);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::VBCM:
		{
			auto* vbcm = dynamic_cast<::RTC::RTCP::FeedbackPsVbcmPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsVbcm::Fuzz(vbcm);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::PSLEI:
		{
			auto* lei = dynamic_cast<::RTC::RTCP::FeedbackPsLeiPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsLei::Fuzz(lei);

			break;
		}

		case ::RTC::RTCP::FeedbackPs::MessageType::AFB:
		{
			auto* afb = dynamic_cast<::RTC::RTCP::FeedbackPsAfbPacket*>(fbps);

			Fuzzer::RTC::RTCP::FeedbackPsAfb::Fuzz(afb);

			break;
		}

		default:
		{
		}
	}
}
