#define MS_CLASS "TEST::RTCP"

#include "fct.h"
#include "RTC/RTCP/Packet.h"
#include "RTC/RTCP/Sdes.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "Logger.h"
#include <string>

using namespace RTC::RTCP;

FCTMF_SUITE_BGN(test_rtcp)
{
	FCT_TEST_BGN(buffer_is_too_small)
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x00, // RTCP common header
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_chk(packet == nullptr);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(version_is_zero)
	{
		uint8_t buffer[] =
		{
			0x00, 0xca, 0x00, 0x01, // RTCP common header
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_chk(packet == nullptr);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(length_is_wrong)
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x04, // RTCP common header
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_chk(packet == nullptr);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(type_is_unknown)
	{
		uint8_t buffer[] =
		{
			0x81, 0x00, 0x00, 0x01, // RTCP common header
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_chk(packet == nullptr);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(sdes_packet)
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x04, // RTCP common header
			0x00, 0x00, 0x00, 0x00, // SDES SSRC
			0x01, 0x0a, 0x6f, 0x75, // SDES Item
			0x74, 0x43, 0x68, 0x61,
			0x6e, 0x6e, 0x65, 0x6c
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_req(packet != nullptr);
		fct_chk(packet->GetType() == Type::SDES);

		SdesPacket* sdes = static_cast<SdesPacket*>(packet);
		SdesChunk* chunk = *(sdes->Begin());
		fct_chk_eq_int(chunk->GetSsrc(), 0);

		chunk->SetSsrc(1234);
		fct_chk_eq_int(chunk->GetSsrc(), 1234);

		SdesItem* item = *(chunk->Begin());
		fct_chk(item->GetType() == SdesItem::Type::CNAME);
		fct_chk_eq_int(item->GetLength(), 10);
		fct_chk_incl_str(item->GetValue(), "outChannel");
	}
	FCT_TEST_END()

	FCT_TEST_BGN(sender_report)
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2,	// ssrc
			0x00, 0x00, 0x04, 0xD2,	// ntp sec
			0x00, 0x00, 0x04, 0xD2,	// ntp frac
			0x00, 0x00, 0x04, 0xD2,	// rtp ts
			0x00, 0x00, 0x04, 0xD2,	// packet count
			0x00, 0x00, 0x04, 0xD2,	// octet count
		};

		SenderReport* report = SenderReport::Parse(buffer, sizeof(SenderReport::Header));
		fct_req(report != nullptr);

		fct_chk_eq_int(report->GetSsrc(), 1234);
		fct_chk_eq_int(report->GetNtpSec(), 1234);
		fct_chk_eq_int(report->GetNtpFrac(), 1234);
		fct_chk_eq_int(report->GetRtpTs(), 1234);
		fct_chk_eq_int(report->GetPacketCount(), 1234);
		fct_chk_eq_int(report->GetOctetCount(), 1234);

		report->SetSsrc(1111);
		report->SetNtpSec(1111);
		report->SetNtpFrac(1111);
		report->SetRtpTs(1111);
		report->SetPacketCount(1111);
		report->SetOctetCount(1111);

		fct_chk_eq_int(report->GetSsrc(), 1111);
		fct_chk_eq_int(report->GetNtpSec(), 1111);
		fct_chk_eq_int(report->GetNtpFrac(), 1111);
		fct_chk_eq_int(report->GetRtpTs(), 1111);
		fct_chk_eq_int(report->GetPacketCount(), 1111);
		fct_chk_eq_int(report->GetOctetCount(), 1111);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(receiver_report)
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2,	// ssrc
			0x01,			// fraction_lost
			0x00, 0x00, 0x04,	// total_lost
			0x00, 0x00, 0x04, 0xD2,	// last_sec
			0x00, 0x00, 0x04, 0xD2,	// jitter
			0x00, 0x00, 0x04, 0xD2,	// lsr
			0x00, 0x00, 0x04, 0xD2,	// dlsr
		};

		ReceiverReport* report = ReceiverReport::Parse(buffer, sizeof(ReceiverReport::Header));
		fct_req(report != nullptr);

		fct_chk_eq_int(report->GetSsrc(), 1234);
		fct_chk_eq_int(report->GetFractionLost(), 1);
		fct_chk_eq_int(report->GetTotalLost(), 4);
		fct_chk_eq_int(report->GetLastSec(), 1234);
		fct_chk_eq_int(report->GetJitter(), 1234);
		fct_chk_eq_int(report->GetLastSenderReport(), 1234);
		fct_chk_eq_int(report->GetDelaySinceLastSenderReport(), 1234);

		report->SetSsrc(1111);
		report->SetFractionLost(2);
		report->SetTotalLost(20);
		report->SetLastSec(1111);
		report->SetJitter(1111);
		report->SetLastSenderReport(1111);
		report->SetDelaySinceLastSenderReport(1111);

		fct_chk_eq_int(report->GetSsrc(), 1111);
		fct_chk_eq_int(report->GetFractionLost(), 2);
		fct_chk_eq_int(report->GetTotalLost(), 20);
		fct_chk_eq_int(report->GetLastSec(), 1111);
		fct_chk_eq_int(report->GetJitter(), 1111);
		fct_chk_eq_int(report->GetLastSenderReport(), 1111);
		fct_chk_eq_int(report->GetDelaySinceLastSenderReport(), 1111);
	}
	FCT_TEST_END()
}
FCTMF_SUITE_END();
