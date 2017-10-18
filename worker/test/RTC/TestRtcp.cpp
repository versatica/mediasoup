#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/Bye.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsLei.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/FeedbackPsRpsi.hpp"
#include "RTC/RTCP/FeedbackPsSli.hpp"
#include "RTC/RTCP/FeedbackPsTst.hpp"
#include "RTC/RTCP/FeedbackPsVbcm.hpp"
#include "RTC/RTCP/FeedbackRtpEcn.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTllei.hpp"
#include "RTC/RTCP/FeedbackRtpTmmb.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <string>

using namespace RTC::RTCP;

SCENARIO("parse RTCP packets", "[parser][rtcp]")
{
	SECTION("minimum header")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x00 // RTCP common header
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		delete packet;
	}

	SECTION("buffer is to small")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("version is zero")
	{
		uint8_t buffer[] =
		{
			0x00, 0xca, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("length is wrong")
	{
		uint8_t buffer[] =
		{
			0x81, 0xca, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("type is unknown")
	{
		uint8_t buffer[] =
		{
			0x81, 0x00, 0x00, 0x01,
			0x00, 0x00, 0x00, 0x00
		};

		Packet* packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE_FALSE(packet);

		delete packet;
	}

	SECTION("parse SdesChunk")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SDES SSRC
			0x01, 0x0a, 0x6f, 0x75, // SDES Item
			0x74, 0x43, 0x68, 0x61,
			0x6e, 0x6e, 0x65, 0x6c
		};

		uint32_t ssrc       = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		std::string value   = "outChannel";
		size_t len          = value.size();
		SdesChunk* chunk    = SdesChunk::Parse(buffer, sizeof(buffer));

		REQUIRE(chunk->GetSsrc() == ssrc);

		SdesItem* item = *(chunk->Begin());

		REQUIRE(item->GetType() == type);
		REQUIRE(item->GetLength() == len);
		REQUIRE(std::string(item->GetValue(), len) == "outChannel");

		delete chunk;
	}

	SECTION("create SdesChunk")
	{
		uint32_t ssrc       = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		std::string value   = "outChannel";
		size_t len          = value.size();
		// Create sdes item.
		SdesItem* item = new SdesItem(type, len, value.c_str());

		// Create sdes chunk.
		SdesChunk chunk(ssrc);

		chunk.AddItem(item);

		// Check chunk content.
		REQUIRE(chunk.GetSsrc() == ssrc);

		// Check item content.
		item = *(chunk.Begin());

		REQUIRE(item->GetType() == type);
		REQUIRE(item->GetLength() == len);
		REQUIRE(std::string(item->GetValue(), len) == "outChannel");
	}

	SECTION("parse SenderReport")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2, // ssrc
			0x00, 0x00, 0x04, 0xD2, // ntp sec
			0x00, 0x00, 0x04, 0xD2, // ntp frac
			0x00, 0x00, 0x04, 0xD2, // rtp ts
			0x00, 0x00, 0x04, 0xD2, // packet count
			0x00, 0x00, 0x04, 0xD2, // octet count
		};

		uint32_t ssrc        = 1234;
		uint32_t ntpSec      = 1234;
		uint32_t ntpFrac     = 1234;
		uint32_t rtpTs       = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount  = 1234;
		SenderReport* report = SenderReport::Parse(buffer, sizeof(SenderReport::Header));

		REQUIRE(report);

		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetNtpSec() == ntpSec);
		REQUIRE(report->GetNtpFrac() == ntpFrac);
		REQUIRE(report->GetRtpTs() == rtpTs);
		REQUIRE(report->GetPacketCount() == packetCount);
		REQUIRE(report->GetOctetCount() == octetCount);

		delete report;
	}

	SECTION("create SenderReport")
	{
		uint32_t ssrc        = 1234;
		uint32_t ntpSec      = 1234;
		uint32_t ntpFrac     = 1234;
		uint32_t rtpTs       = 1234;
		uint32_t packetCount = 1234;
		uint32_t octetCount  = 1234;
		// Create local report and check content.
		// SenderReport();
		SenderReport report1;

		report1.SetSsrc(ssrc);
		report1.SetNtpSec(ntpSec);
		report1.SetNtpFrac(ntpFrac);
		report1.SetRtpTs(rtpTs);
		report1.SetPacketCount(packetCount);
		report1.SetOctetCount(octetCount);

		REQUIRE(report1.GetSsrc() == ssrc);
		REQUIRE(report1.GetNtpSec() == ntpSec);
		REQUIRE(report1.GetNtpFrac() == ntpFrac);
		REQUIRE(report1.GetRtpTs() == rtpTs);
		REQUIRE(report1.GetPacketCount() == packetCount);
		REQUIRE(report1.GetOctetCount() == octetCount);

		// Create report out of the existing one and check content.
		// SenderReport(SenderReport* report);
		SenderReport report2(&report1);

		REQUIRE(report2.GetSsrc() == ssrc);
		REQUIRE(report2.GetNtpSec() == ntpSec);
		REQUIRE(report2.GetNtpFrac() == ntpFrac);
		REQUIRE(report2.GetRtpTs() == rtpTs);
		REQUIRE(report2.GetPacketCount() == packetCount);
		REQUIRE(report2.GetOctetCount() == octetCount);
	}

	SECTION("parse ReceiverReport")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x04, 0xD2,	// ssrc
			0x01,                   // fractionLost
			0x00, 0x00, 0x04,       // totalLost
			0x00, 0x00, 0x04, 0xD2,	// lastSeq
			0x00, 0x00, 0x04, 0xD2,	// jitter
			0x00, 0x00, 0x04, 0xD2,	// lsr
			0x00, 0x00, 0x04, 0xD2  // dlsr
		};

		uint32_t ssrc                       = 1234;
		uint8_t fractionLost                = 1;
		int32_t totalLost                   = 4;
		uint32_t lastSeq                    = 1234;
		uint32_t jitter                     = 1234;
		uint32_t lastSenderReport           = 1234;
		uint32_t delaySinceLastSenderReport = 1234;
		ReceiverReport* report = ReceiverReport::Parse(buffer, sizeof(ReceiverReport::Header));

		REQUIRE(report);
		REQUIRE(report->GetSsrc() == ssrc);
		REQUIRE(report->GetFractionLost() == fractionLost);
		REQUIRE(report->GetTotalLost() == totalLost);
		REQUIRE(report->GetLastSeq() == lastSeq);
		REQUIRE(report->GetJitter() == jitter);
		REQUIRE(report->GetLastSenderReport() == lastSenderReport);
		REQUIRE(report->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}

	SECTION("create ReceiverReport")
	{
		uint32_t ssrc                       = 1234;
		uint8_t fractionLost                = 1;
		int32_t totalLost                   = 4;
		uint32_t lastSeq                    = 1234;
		uint32_t jitter                     = 1234;
		uint32_t lastSenderReport           = 1234;
		uint32_t delaySinceLastSenderReport = 1234;
		// Create local report and check content.
		// ReceiverReport();
		ReceiverReport report1;

		report1.SetSsrc(ssrc);
		report1.SetFractionLost(fractionLost);
		report1.SetTotalLost(totalLost);
		report1.SetLastSeq(lastSeq);
		report1.SetJitter(jitter);
		report1.SetLastSenderReport(lastSenderReport);
		report1.SetDelaySinceLastSenderReport(delaySinceLastSenderReport);

		REQUIRE(report1.GetSsrc() == ssrc);
		REQUIRE(report1.GetFractionLost() == fractionLost);
		REQUIRE(report1.GetTotalLost() == totalLost);
		REQUIRE(report1.GetLastSeq() == lastSeq);
		REQUIRE(report1.GetJitter() == jitter);
		REQUIRE(report1.GetLastSenderReport() == lastSenderReport);
		REQUIRE(report1.GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);

		// Create report out of the existing one and check content.
		// ReceiverReport(ReceiverReport* report);
		ReceiverReport report2(&report1);

		REQUIRE(report2.GetSsrc() == ssrc);
		REQUIRE(report2.GetFractionLost() == fractionLost);
		REQUIRE(report2.GetTotalLost() == totalLost);
		REQUIRE(report2.GetLastSeq() == lastSeq);
		REQUIRE(report2.GetJitter() == jitter);
		REQUIRE(report2.GetLastSenderReport() == lastSenderReport);
		REQUIRE(report2.GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}

	SECTION("parse SenderReport")
	{
		uint8_t buffer[] =
		{
			0x81, 0xc8, 0x00, 0x0c, // Type: 200 (Sender Report), Count: 1, Length: 12
			0x5d, 0x93, 0x15, 0x34, // SSRC: 0x5d931534
			0xdd, 0x3a, 0xc1, 0xb4, // NTP Sec: 3711615412
			0x76, 0x54, 0x71, 0x71, // NTP Frac: 1985245553
			0x00, 0x08, 0xcf, 0x00, // RTP timestamp: 577280
			0x00, 0x00, 0x0e, 0x18, // Packet count: 3608
			0x00, 0x08, 0xcf, 0x00, // Octed count: 577280
			                        // Receiver Report
			0x01, 0x93, 0x2d, 0xb4, // SSRC. 0x01932db4
			0x00, 0x00, 0x00, 0x01, // Fraction lost: 0, Total lost: 1
			0x00, 0x00, 0x00, 0x00, // Extended highest sequence number: 0
			0x00, 0x00, 0x00, 0x00, // Jitter: 0
			0x00, 0x00, 0x00, 0x00, // Last SR: 0
			0x00, 0x00, 0x00, 0x05  // DLSR: 0
		};

		uint32_t ssrc        = 0x5d931534;
		uint32_t ntpSec      = 3711615412;
		uint32_t ntpFrac     = 1985245553;
		uint32_t rtpTs       = 577280;
		uint32_t packetCount = 3608;
		uint32_t octetCount  = 577280;

		auto packet = Packet::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);

		auto sr = dynamic_cast<SenderReportPacket*>(packet);
		auto sIt     = sr->Begin();
		auto sReport = *sIt;

		REQUIRE(sReport->GetSsrc() == ssrc);
		REQUIRE(sReport->GetNtpSec() == ntpSec);
		REQUIRE(sReport->GetNtpFrac() == ntpFrac);
		REQUIRE(sReport->GetRtpTs() == rtpTs);
		REQUIRE(sReport->GetPacketCount() == packetCount);
		REQUIRE(sReport->GetOctetCount() == octetCount);

		REQUIRE(packet->GetNext());

		auto rr      = dynamic_cast<ReceiverReportPacket*>(packet->GetNext());
		auto rIt     = rr->Begin();
		auto rReport = *rIt;

		ssrc                                = 0x01932db4;

		uint8_t fractionLost                = 0;
		uint8_t totalLost                   = 1;
		uint32_t lastSeq                    = 0;
		uint32_t jitter                     = 0;
		uint32_t lastSenderReport           = 0;
		uint32_t delaySinceLastSenderReport = 5;

		REQUIRE(rReport);
		REQUIRE(rReport->GetSsrc() == ssrc);
		REQUIRE(rReport->GetFractionLost() == fractionLost);
		REQUIRE(rReport->GetTotalLost() == totalLost);
		REQUIRE(rReport->GetLastSeq() == lastSeq);
		REQUIRE(rReport->GetJitter() == jitter);
		REQUIRE(rReport->GetLastSenderReport() == lastSenderReport);
		REQUIRE(rReport->GetDelaySinceLastSenderReport() == delaySinceLastSenderReport);
	}

	SECTION("create ByePacket")
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

		REQUIRE(*it == ssrc1);
		it++;
		REQUIRE(*it == ssrc2);
		REQUIRE(bye1.GetReason() == reason);

		// Locally store the content of the packet.
		uint8_t buffer[bye1.GetSize()];

		bye1.Serialize(buffer);

		// Parse the buffer of the previous packet and check content.
		ByePacket* bye2 = ByePacket::Parse(buffer, sizeof(buffer));

		it = bye2->Begin();

		REQUIRE(*it == ssrc1);
		it++;
		REQUIRE(*it == ssrc2);
		REQUIRE(bye2->GetReason() == reason);
	}

	SECTION("parse FeedbackRtpNackItem")
	{
		uint8_t buffer[] =
		{
			0x09, 0xc4, 0b10101010, 0b01010101
		};
		uint16_t packetId = 2500;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		FeedbackRtpNackItem* item  = FeedbackRtpNackItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPacketId() == packetId);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);

		delete item;
	}

	SECTION("create FeedbackRtpNackItem")
	{
		uint16_t packetId          = 1;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		// Create local NackItem and check content.
		// FeedbackRtpNackItem();
		FeedbackRtpNackItem item1(packetId, lostPacketBitmask);

		REQUIRE(item1.GetPacketId() == packetId);
		REQUIRE(item1.GetLostPacketBitmask() == lostPacketBitmask);

		// Create local NackItem out of existing one and check content.
		// FeedbackRtpNackItem(FeedbackRtpNackItem*);
		FeedbackRtpNackItem item2(&item1);

		REQUIRE(item2.GetPacketId() == packetId);
		REQUIRE(item2.GetLostPacketBitmask() == lostPacketBitmask);

		// Locally store the content of the packet.
		uint8_t buffer[item2.GetSize()];

		item2.Serialize(buffer);

		// Create local NackItem out of previous packet buffer and check content.
		// FeedbackRtpNackItem(FeedbackRtpNackItem::Header*);
		FeedbackRtpNackItem item3((FeedbackRtpNackItem::Header*)buffer);

		REQUIRE(item3.GetPacketId() == packetId);
		REQUIRE(item3.GetLostPacketBitmask() == lostPacketBitmask);
	}

	SECTION("create FeedbackRtpTmmbrItem")
	{
		uint32_t ssrc     = 1234;
		uint64_t bitrate  = 3000000; // bits per second.
		uint32_t overhead = 1;
		FeedbackRtpTmmbrItem item;

		item.SetSsrc(ssrc);
		item.SetBitrate(bitrate);
		item.SetOverhead(overhead);

		REQUIRE(item.GetSsrc() == ssrc);
		REQUIRE(item.GetBitrate() == bitrate);
		REQUIRE(item.GetOverhead() == overhead);

		uint8_t buffer[8];

		item.Serialize(buffer);

		FeedbackRtpTmmbrItem* item2 = FeedbackRtpTmmbrItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item2);
		REQUIRE(item2->GetSsrc() == ssrc);
		REQUIRE(item2->GetBitrate() == bitrate);
		REQUIRE(item2->GetOverhead() == overhead);

		delete item2;
	}

	SECTION("parse FeedbackRtpTmmbrItem")
	{
		uint8_t buffer[] =
		{
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		uint32_t ssrc     = 3131870413;
		uint64_t bitrate  = 365504;
		uint16_t overhead = 0;
		FeedbackRtpTmmbrItem* item = FeedbackRtpTmmbrItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetBitrate() == bitrate);
		REQUIRE(item->GetOverhead() == overhead);

		delete item;
	}

	SECTION("parse FeedbackRtpTmmbrPacket")
	{

		uint8_t buffer[] =
		{
			0x83, 0xcd, 0x00, 0x04,
			0x6d, 0x6a, 0x8c, 0x9f,
			0x00, 0x00, 0x00, 0x00,
			0xba, 0xac, 0x8c, 0xcd,
			0x18, 0x2c, 0x9e, 0x00
		};
		uint32_t ssrc     = 3131870413;
		uint64_t bitrate  = 365504;
		uint16_t overhead = 0;
		FeedbackRtpTmmbrPacket* packet = FeedbackRtpTmmbrPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);
		REQUIRE(packet->Begin() != packet->End());

		FeedbackRtpTmmbrItem* item = (*packet->Begin());

		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetBitrate() == bitrate);
		REQUIRE(item->GetOverhead() == overhead);

		delete packet;
	}

	SECTION("parse FeedbackRtpTlleiItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x01, 0b10101010, 0b01010101
		};
		uint16_t packetId = 1;
		uint16_t lostPacketBitmask = 0b1010101001010101;
		FeedbackRtpTlleiItem* item = FeedbackRtpTlleiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPacketId() == packetId);
		REQUIRE(item->GetLostPacketBitmask() == lostPacketBitmask);

		delete item;
	}

	SECTION("parse FeedbackRtpEcnItem")
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
		FeedbackRtpEcnItem* item = FeedbackRtpEcnItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSequenceNumber() == sequenceNumber);
		REQUIRE(item->GetEct0Counter() == ect0Counter);
		REQUIRE(item->GetEct1Counter() == ect1Counter);
		REQUIRE(item->GetEcnCeCounter() == ecnCeCounter);
		REQUIRE(item->GetNotEctCounter() == notEctCounter);
		REQUIRE(item->GetLostPackets() == lostPackets);
		REQUIRE(item->GetDuplicatedPackets() == duplicatedPackets);

		delete item;
	}

	SECTION("parse FeedbackPsSliItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x08, 0x01, 0x01
		};
		uint16_t first = 1;
		uint16_t number = 4;
		uint8_t pictureId = 1;
		FeedbackPsSliItem* item = FeedbackPsSliItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetFirst() == first);
		REQUIRE(item->GetNumber() == number);
		REQUIRE(item->GetPictureId() == pictureId);

		delete item;
	}

	SECTION("parse FeedbackPsRpsiItem")
	{
		uint8_t buffer[] =
		{
			0x08,                   // Padding Bits
			0x02,                   // Zero | Payload Type
			0x00, 0x00,             // Native RPSI bit string
			0x00, 0x00, 0x01, 0x00
		};
		uint8_t payloadType = 1;
		uint8_t payloadMask = 1;
		size_t length = 5;
		FeedbackPsRpsiItem* item = FeedbackPsRpsiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetBitString()[item->GetLength()-1] & 1) == payloadMask);

		delete item;
	}

	SECTION("parse FeedbackPsFirItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08, 0x00, 0x00, 0x00 // Seq nr.
		};
		uint32_t ssrc = 0;
		uint8_t seq = 8;
		FeedbackPsFirItem* item = FeedbackPsFirItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);

		delete item;
	}

	SECTION("parse FeedbackPsTstnItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08,                   // Seq nr.
			0x00, 0x00, 0x08        // Reserved | Index
		};
		uint32_t ssrc = 0;
		uint8_t seq = 8;
		uint8_t index = 1;
		FeedbackPsTstnItem* item = FeedbackPsTstnItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
		REQUIRE(item->GetIndex() == index);

		delete item;
	}

	SECTION("parse FeedbackPsVbcmItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SSRC
			0x08,                   // Seq nr.
			0x02,                   // Zero | Payload Vbcm
			0x00, 0x01,             // Length
			0x01,                   // VBCM Octet String
			0x00, 0x00, 0x00        // Padding
		};
		uint32_t ssrc = 0;
		uint8_t seq = 8;
		uint8_t payloadType = 1;
		uint16_t length = 1;
		uint8_t valueMask = 1;
		FeedbackPsVbcmItem* item = FeedbackPsVbcmItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);
		REQUIRE(item->GetSequenceNumber() == seq);
		REQUIRE(item->GetPayloadType() == payloadType);
		REQUIRE(item->GetLength() == length);
		REQUIRE((item->GetValue()[item->GetLength() -1] & 1) == valueMask);

		delete item;
	}

	SECTION("parse FeedbackPsLeiItem")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x01 // SSRC
		};
		uint32_t ssrc = 1;
		FeedbackPsLeiItem* item = FeedbackPsLeiItem::Parse(buffer, sizeof(buffer));

		REQUIRE(item);
		REQUIRE(item->GetSsrc() == ssrc);

		delete item;
	}

	SECTION("parse FeedbackPsAfbPacket")
	{
		uint8_t buffer[] =
		{
			0x8F, 0xce, 0x00, 0x03, // RTCP common header
			0x00, 0x00, 0x00, 0x00, // Sender SSRC
			0x00, 0x00, 0x00, 0x00, // Media SSRC
			0x00, 0x00, 0x00, 0x01  // Data
		};
		size_t dataSize = 4;
		uint8_t dataBitmask = 1;
		FeedbackPsAfbPacket* packet = FeedbackPsAfbPacket::Parse(buffer, sizeof(buffer));

		REQUIRE(packet);
		REQUIRE(packet->GetApplication() == FeedbackPsAfbPacket::Application::UNKNOWN);

		delete packet;
	}

	SECTION("FeedbackPsRembPacket")
	{
		uint32_t sender_ssrc = 0;
		uint32_t media_ssrc = 0;
		uint64_t bitrate = 654321;
		// Precission lost.
		uint64_t bitrateParsed = 654320;
		std::vector<uint32_t> ssrcs { 11111, 22222, 33333, 44444 };
		// Create a packet.
		FeedbackPsRembPacket packet(sender_ssrc, media_ssrc);

		packet.SetBitrate(bitrate);
		packet.SetSsrcs(ssrcs);

		// Serialize.
		uint8_t rtcpBuffer[RTC::RTCP::BufferSize];

		packet.Serialize(rtcpBuffer);

		RTC::RTCP::Packet::CommonHeader* header = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(rtcpBuffer);
		size_t len = (size_t)(ntohs(header->length) + 1) * 4;

		// Recover the packet out of the serialized buffer.
		FeedbackPsRembPacket* parsed = FeedbackPsRembPacket::Parse(rtcpBuffer, len);

		REQUIRE(parsed);
		REQUIRE(parsed->GetMediaSsrc() == media_ssrc);
		REQUIRE(parsed->GetSenderSsrc() == sender_ssrc);
		REQUIRE(parsed->GetBitrate() == bitrateParsed);
		REQUIRE(parsed->GetSsrcs() == ssrcs);
	}
}
