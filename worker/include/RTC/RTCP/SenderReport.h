#ifndef MS_RTC_RTCP_SENDER_REPORT_H
#define MS_RTC_RTCP_SENDER_REPORT_H

#include "common.h"
#include "RTC/RTCP/Packet.h"
#include <vector>

namespace RTC { namespace RTCP
{
	class SenderReport
	{
	public:
		/* Struct for RTCP sender report. */
		struct Header
		{
			uint32_t ssrc;
			uint32_t ntp_sec;
			uint32_t ntp_frac;
			uint32_t rtp_ts;
			uint32_t packet_count;
			uint32_t octet_count;
		};

	public:
		static SenderReport* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		SenderReport(Header* header);
		SenderReport(SenderReport* report);

		// Locally generated Report. Holds the data internally.
		SenderReport();
		~SenderReport();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();
		const uint8_t* GetRaw();
		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		uint32_t GetNtpSec();
		void SetNtpSec(uint32_t ntp_sec);
		uint32_t GetNtpFrac();
		void SetNtpFrac(uint32_t ntp_frac);
		uint32_t GetRtpTs();
		void SetRtpTs(uint32_t rtp_ts);
		uint32_t GetPacketCount();
		void SetPacketCount(uint32_t packet_count);
		uint32_t GetOctetCount();
		void SetOctetCount(uint32_t octet_count);

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;
	};

	class SenderReportPacket
		: public Packet
	{
	public:
		typedef std::vector<SenderReport*>::iterator Iterator;

	public:
		static SenderReportPacket* Parse(const uint8_t* data, size_t len);

	public:
		SenderReportPacket();
		~SenderReportPacket();

		// Virtual methods inherited from Packet
		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetCount() override;
		size_t GetSize() override;

		void AddReport(SenderReport* report);
		Iterator Begin();
		Iterator End();

	private:
		std::vector<SenderReport*> reports;
	};

	/* SenderReport inline instance methods. */

	inline
	SenderReport::SenderReport(Header* header) :
		header(header)
	{
	}

	inline
	SenderReport::SenderReport(SenderReport* report) :
		header(report->header)
	{
	}

	inline
	SenderReport::~SenderReport()
	{
		if (this->raw)
			delete this->raw;
	}

	inline
	size_t SenderReport::GetSize()
	{
		return sizeof(Header);
	}

	inline
	const uint8_t* SenderReport::GetRaw()
	{
		return this->raw;
	}

	inline
	uint32_t SenderReport::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	void SenderReport::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	uint32_t SenderReport::GetNtpSec()
	{
		return (uint32_t)ntohl(this->header->ntp_sec);
	}

	inline
	void SenderReport::SetNtpSec(uint32_t ntp_sec)
	{
		this->header->ntp_sec = (uint32_t)htonl(ntp_sec);
	}

	inline
	uint32_t SenderReport::GetNtpFrac()
	{
		return (uint32_t)ntohl(this->header->ntp_frac);
	}

	inline
	void SenderReport::SetNtpFrac(uint32_t ntp_frac)
	{
		this->header->ntp_frac = (uint32_t)htonl(ntp_frac);
	}

	inline
	uint32_t SenderReport::GetRtpTs()
	{
		return (uint32_t)ntohl(this->header->rtp_ts);
	}

	inline
	void SenderReport::SetRtpTs(uint32_t rtp_ts)
	{
		this->header->rtp_ts = (uint32_t)htonl(rtp_ts);
	}

	inline

	uint32_t SenderReport::GetPacketCount()
	{
		return (uint32_t)ntohl(this->header->packet_count);
	}

	inline
	void SenderReport::SetPacketCount(uint32_t packet_count)
	{
		this->header->packet_count = (uint32_t)htonl(packet_count);
	}

	inline
	uint32_t SenderReport::GetOctetCount()
	{
		return (uint32_t)ntohl(this->header->octet_count);
	}

	inline
	void SenderReport::SetOctetCount(uint32_t octet_count)
	{
		this->header->octet_count = (uint32_t)htonl(octet_count);
	}

	/* SenderReportPacket Inline instance methods. */

	inline
	SenderReportPacket::SenderReportPacket()
		: Packet(Type::SR)
	{
	}

	inline
	SenderReportPacket::~SenderReportPacket()
	{
		for(auto report : this->reports)
		{
			delete report;
		}
	}

	inline
	size_t SenderReportPacket::GetCount()
	{
		return this->reports.size();
	}

	inline
	size_t SenderReportPacket::GetSize()
	{
		size_t size = sizeof(Packet::CommonHeader);

		for (auto report : this->reports)
		{
			size += report->GetSize();
		}

		return size;
	}

	inline
	void SenderReportPacket::AddReport(SenderReport* report)
	{
		this->reports.push_back(report);
	}

	inline
	SenderReportPacket::Iterator SenderReportPacket::Begin()
	{
		return this->reports.begin();
	}

	inline
	SenderReportPacket::Iterator SenderReportPacket::End()
	{
		return this->reports.end();
	}

} } // RTP::RTCP

#endif
