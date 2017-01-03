#define MS_CLASS "TEST::RTCP"

#include "fct.h"
#include "common.h"
#include "RTC/RTCP/Packet.h"
#include "RTC/RTCP/Sdes.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/Bye.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "RTC/RTCP/FeedbackRtpTllei.h"
#include "RTC/RTCP/FeedbackRtpEcn.h"
#include "RTC/RTCP/FeedbackPsSli.h"
#include "RTC/RTCP/FeedbackPsRpsi.h"
#include "Logger.h"
#include <string>

using namespace RTC::RTCP;

FCTMF_SUITE_BGN(test_rtcp)
{
	FCT_TEST_BGN(minimum_header)
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x00, // RTCP common header
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));
		fct_chk(packet != nullptr);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(buffer_is_too_small)
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00
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

	FCT_TEST_BGN(parse_sdes_chunk)
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SDES SSRC
			0x01, 0x0a, 0x6f, 0x75, // SDES Item
			0x74, 0x43, 0x68, 0x61,
			0x6e, 0x6e, 0x65, 0x6c
		};

		uint32_t ssrc = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		const char* value = "outChannel";
		size_t len = strlen(value);

		SdesChunk* chunk = SdesChunk::Parse(buffer, sizeof(buffer));
		fct_chk_eq_int(chunk->GetSsrc(), ssrc);

		SdesItem* item = *(chunk->Begin());
		fct_chk(item->GetType() == type);
		fct_chk_eq_int(item->GetLength(), len);
		fct_chk_incl_str(item->GetValue(), value);

		delete chunk;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(create_sdes_chunk)
	{
		uint32_t ssrc = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		const char* value = "outChannel";
		size_t len = strlen(value);

		// Create sdes item.
		SdesItem* item = new SdesItem(type, len, value);

		// Create sdes chunk.
		SdesChunk chunk(ssrc);
		chunk.AddItem(item);

		// Check chunk content.
		fct_chk_eq_int(chunk.GetSsrc(), ssrc);

		// Check item content.
		item = *(chunk.Begin());
		fct_chk(item->GetType() == type);
		fct_chk_eq_int(item->GetLength(), len);
		fct_chk_incl_str(item->GetValue(), value);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_sender_report)
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

		uint32_t ssrc = 1234;
		uint32_t ntpSec = 1234;
		uint32_t ntpFrac = 1234;
		uint32_t rtpTs = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount = 1234;

		SenderReport* report = SenderReport::Parse(buffer, sizeof(SenderReport::Header));
		fct_req(report != nullptr);

		fct_chk_eq_int(report->GetSsrc(), ssrc);
		fct_chk_eq_int(report->GetNtpSec(), ntpSec);
		fct_chk_eq_int(report->GetNtpFrac(), ntpFrac);
		fct_chk_eq_int(report->GetRtpTs(), rtpTs);
		fct_chk_eq_int(report->GetPacketCount(), packetCount);
		fct_chk_eq_int(report->GetOctetCount(), octetCount);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(create_sender_report)
	{
		uint32_t ssrc = 1234;
		uint32_t ntpSec = 1234;
		uint32_t ntpFrac = 1234;
		uint32_t rtpTs = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount = 1234;

		// Create local report and check content.
		// SenderReport();
		SenderReport report1;

		report1.SetSsrc(ssrc);
		report1.SetNtpSec(ntpSec);
		report1.SetNtpFrac(ntpFrac);
		report1.SetRtpTs(rtpTs);
		report1.SetPacketCount(packetCount);
		report1.SetOctetCount(octetCount);

		fct_chk_eq_int(report1.GetSsrc(), ssrc);
		fct_chk_eq_int(report1.GetNtpSec(), ntpSec);
		fct_chk_eq_int(report1.GetNtpFrac(), ntpFrac);
		fct_chk_eq_int(report1.GetRtpTs(), rtpTs);
		fct_chk_eq_int(report1.GetPacketCount(), packetCount);
		fct_chk_eq_int(report1.GetOctetCount(), octetCount);

		// Create report out of the existing one and check content.
		// SenderReport(SenderReport* report);
		SenderReport report2(&report1);

		fct_chk_eq_int(report2.GetSsrc(), ssrc);
		fct_chk_eq_int(report2.GetNtpSec(), ntpSec);
		fct_chk_eq_int(report2.GetNtpFrac(), ntpFrac);
		fct_chk_eq_int(report2.GetRtpTs(), rtpTs);
		fct_chk_eq_int(report2.GetPacketCount(), packetCount);
		fct_chk_eq_int(report2.GetOctetCount(), octetCount);

		// Locally store the content of the report.
		report2.Serialize();

		// Create report out of buffer and check content.
		// SenderReport(Header* header);
		SenderReport report3((SenderReport::Header*)report2.GetRaw());

		fct_chk_eq_int(report3.GetSsrc(), ssrc);
		fct_chk_eq_int(report3.GetNtpSec(), ntpSec);
		fct_chk_eq_int(report3.GetNtpFrac(), ntpFrac);
		fct_chk_eq_int(report3.GetRtpTs(), rtpTs);
		fct_chk_eq_int(report3.GetPacketCount(), packetCount);
		fct_chk_eq_int(report3.GetOctetCount(), octetCount);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_receiver_report)
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

		uint32_t ssrc = 1234;
		uint8_t fractionLost = 1;
		int32_t totalLost = 4;
		uint32_t lastSec = 1234;
		uint32_t jitter = 1234;
		uint32_t lastSenderReport = 1234;
		uint32_t delaySinceLastSenderReport = 1234;

		ReceiverReport* report = ReceiverReport::Parse(buffer, sizeof(ReceiverReport::Header));
		fct_req(report != nullptr);

		fct_chk_eq_int(report->GetSsrc(), ssrc);
		fct_chk_eq_int(report->GetFractionLost(), fractionLost);
		fct_chk_eq_int(report->GetTotalLost(), totalLost);
		fct_chk_eq_int(report->GetLastSec(), lastSec);
		fct_chk_eq_int(report->GetJitter(), jitter);
		fct_chk_eq_int(report->GetLastSenderReport(), lastSenderReport);
		fct_chk_eq_int(report->GetDelaySinceLastSenderReport(), delaySinceLastSenderReport);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(create_receiver_report)
	{
		uint32_t ssrc = 1234;
		uint8_t fractionLost = 1;
		int32_t totalLost = 4;
		uint32_t lastSec = 1234;
		uint32_t jitter = 1234;
		uint32_t lastSenderReport = 1234;
		uint32_t delaySinceLastSenderReport = 1234;

		// Create local report and check content.
		// ReceiverReport();
		ReceiverReport report1;

		report1.SetSsrc(ssrc);
		report1.SetFractionLost(fractionLost);
		report1.SetTotalLost(totalLost);
		report1.SetLastSec(lastSec);
		report1.SetJitter(jitter);
		report1.SetLastSenderReport(lastSenderReport);
		report1.SetDelaySinceLastSenderReport(delaySinceLastSenderReport);

		fct_chk_eq_int(report1.GetSsrc(), ssrc);
		fct_chk_eq_int(report1.GetFractionLost(), fractionLost);
		fct_chk_eq_int(report1.GetTotalLost(), totalLost);
		fct_chk_eq_int(report1.GetLastSec(), lastSec);
		fct_chk_eq_int(report1.GetJitter(), jitter);
		fct_chk_eq_int(report1.GetLastSenderReport(), lastSenderReport);
		fct_chk_eq_int(report1.GetDelaySinceLastSenderReport(), delaySinceLastSenderReport);

		// Create report out of the existing one and check content.
		// ReceiverReport(ReceiverReport* report);
		ReceiverReport report2(&report1);

		fct_chk_eq_int(report2.GetSsrc(), ssrc);
		fct_chk_eq_int(report2.GetFractionLost(), fractionLost);
		fct_chk_eq_int(report2.GetTotalLost(), totalLost);
		fct_chk_eq_int(report2.GetLastSec(), lastSec);
		fct_chk_eq_int(report2.GetJitter(), jitter);
		fct_chk_eq_int(report2.GetLastSenderReport(), lastSenderReport);
		fct_chk_eq_int(report2.GetDelaySinceLastSenderReport(), delaySinceLastSenderReport);

		// Locally store the content of the report.
		report2.Serialize();

		// Create report out of buffer and check content.
		// ReceiverReport(Header* header);
		ReceiverReport report3((ReceiverReport::Header*)report2.GetRaw());

		fct_chk_eq_int(report2.GetSsrc(), ssrc);
		fct_chk_eq_int(report2.GetFractionLost(), fractionLost);
		fct_chk_eq_int(report2.GetTotalLost(), totalLost);
		fct_chk_eq_int(report2.GetLastSec(), lastSec);
		fct_chk_eq_int(report2.GetJitter(), jitter);
		fct_chk_eq_int(report2.GetLastSenderReport(), lastSenderReport);
		fct_chk_eq_int(report2.GetDelaySinceLastSenderReport(), delaySinceLastSenderReport);

	}
	FCT_TEST_END()

	FCT_TEST_BGN(create_parse_bye)
	{
		uint32_t ssrc1 = 1111;
		uint32_t ssrc2 = 2222;
		std::string reason("hasta la vista");

		// Create local Bye packet and check content.
		// ByePacket();
		ByePacket bye1;

		bye1.AddSsrc(ssrc1);
		bye1.AddSsrc(ssrc2);
		bye1.SetReason(reason);

		ByePacket::Iterator it = bye1.Begin();

		fct_chk_eq_int(*(it), ssrc1);
		fct_chk_eq_int(*(++it), ssrc2);
		fct_chk(bye1.GetReason() == reason);

		// Locally store the content of the packet.
		uint8_t buffer[bye1.GetSize()];
		bye1.Serialize(buffer);

		// Parse the buffer of the previous packet and check content.
		ByePacket* bye2 = ByePacket::Parse(buffer, sizeof(buffer));

		it = bye2->Begin();

		fct_chk_eq_int(*(it), ssrc1);
		fct_chk_eq_int(*(++it), ssrc2);
		fct_chk(bye2->GetReason() == reason);
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_rtpfb_nack_item)
	{
		uint8_t buffer[] =
		{
			0x00, 0x01, 0x02, 0x00
		};

		uint16_t packetId = 1;
		uint16_t lostPacketBitmask = 2;

		NackItem* item = NackItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetPacketId(), packetId);
		fct_chk_eq_int(item->GetLostPacketBitmask(), lostPacketBitmask);

		delete item;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(create_rtpfb_nack_item)
	{
		uint16_t packetId = 1;
		uint16_t lostPacketBitmask = 2;

		// Create local NackItem and check content.
		// NackItem();
		NackItem item1(packetId, lostPacketBitmask);
		fct_chk_eq_int(item1.GetPacketId(), packetId);
		fct_chk_eq_int(item1.GetLostPacketBitmask(), htons(lostPacketBitmask));

		// Create local NackItem out of existing one and check content.
		// NackItem(NackItem*);
		NackItem item2(&item1);
		fct_chk_eq_int(item2.GetPacketId(), packetId);
		fct_chk_eq_int(item2.GetLostPacketBitmask(), htons(lostPacketBitmask));

		// Locally store the content of the packet.
		uint8_t buffer[item2.GetSize()];
		item2.Serialize(buffer);

		// Create local NackItem out of previous packet buffer and check content.
		// NackItem(NackItem::Header*);
		NackItem item3((NackItem::Header*)buffer);
		fct_chk_eq_int(item3.GetPacketId(), packetId);
		fct_chk_eq_int(item3.GetLostPacketBitmask(), htons(lostPacketBitmask));
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_rtpfb_tmmb_item)
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // ssrc
			0x04, 0x00, 0x02, 0x01
		};

		uint32_t ssrc = 0;
		uint64_t bitrate = 2;
		uint32_t overhead = 1;

		TmmbrItem* item = TmmbrItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetSsrc(), ssrc);
		fct_chk(item->GetBitrate() == bitrate);
		fct_chk_eq_int(item->GetOverhead(), overhead);

		delete item;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_rtpfb_tllei_item)
	{
		uint8_t buffer[] =
		{
			0x00, 0x01, 0x02, 0x00
		};

		uint16_t packetId = 1;
		uint16_t lostPacketBitmask = 2;

		TlleiItem* item = TlleiItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetPacketId(), packetId);
		fct_chk_eq_int(item->GetLostPacketBitmask(), lostPacketBitmask);

		delete item;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_rtpfb_ecn_item)
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x01, // Extended Highest Sequence Number
			0x00, 0x00, 0x00, 0x01, // ECT (0) Counter
			0x00, 0x00, 0x00, 0x01, // ECT (1) Counter
			0x00, 0x01,             // ECN-CE Counter
			0x00, 0x01,             // not-ECT Counter
			0x00, 0x01,             // Lost Packets Counter
			0x00, 0x01              // Duplication Counter
		};

		uint32_t sequenceNumber = 1;
		uint32_t ect0Counter = 1;
		uint32_t ect1Counter = 1;
		uint16_t ecnCeCounter = 1;
		uint16_t notEctCounter = 1;
		uint16_t lostPackets = 1;
		uint16_t duplicatedPackets = 1;

		EcnItem* item = EcnItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetSequenceNumber(), sequenceNumber);
		fct_chk_eq_int(item->GetEct0Counter(), ect0Counter);
		fct_chk_eq_int(item->GetEct1Counter(), ect1Counter);
		fct_chk_eq_int(item->GetEcnCeCounter(), ecnCeCounter);
		fct_chk_eq_int(item->GetNotEctCounter(), notEctCounter);
		fct_chk_eq_int(item->GetLostPackets(), lostPackets);
		fct_chk_eq_int(item->GetDuplicatedPackets(), duplicatedPackets);

		delete item;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_psfb_sli_item)
	{
		uint8_t buffer[] =
		{
			0x00, 0x08, 0x01, 0x01
		};

		uint16_t first = 1;
		uint16_t number = 4;
		uint8_t  pictureId = 1;

		SliItem* item = SliItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetFirst(), first);
		fct_chk_eq_int(item->GetNumber(), number);
		fct_chk_eq_int(item->GetPictureId(), pictureId);

		delete item;
	}
	FCT_TEST_END()

	FCT_TEST_BGN(parse_psfb_rpsi_item)
	{
		uint8_t buffer[] =
		{
			0x08,                   // Padding Bits
			0x02,                   // Zero | Payload Type
			0x00, 0x00,             // Native RPSI bit string
			0x00, 0x00, 0x01, 0x00
		};

		uint8_t  payloadType = 1;
		uint8_t  payloadMask = 1;
		size_t length = 5;

		RpsiItem* item = RpsiItem::Parse(buffer, sizeof(buffer));
		fct_req(item != nullptr);

		fct_chk_eq_int(item->GetPayloadType(), payloadType);
		fct_chk_eq_int(item->GetLength(), length);
		fct_chk_eq_int(item->GetBitString()[item->GetLength()-1] & 1, payloadMask);

		delete item;
	}
	FCT_TEST_END()
}
FCTMF_SUITE_END();
