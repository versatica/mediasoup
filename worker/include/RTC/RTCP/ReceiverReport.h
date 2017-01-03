#ifndef MS_RTC_RTCP_RECEIVER_REPORT_H
#define MS_RTC_RTCP_RECEIVER_REPORT_H

#include "common.h"
#include "RTC/RTCP/Packet.h"
#include "Utils.h"
#include <vector>

namespace RTC { namespace RTCP
{
	class ReceiverReport
	{
	public:
		/* Struct for RTCP receiver report. */
		struct Header
		{
			uint32_t ssrc;
			uint32_t fraction_lost:8;
			uint32_t total_lost:24;
			uint32_t last_seq;
			uint32_t jitter;
			uint32_t lsr;
			uint32_t dlsr;
		};

	public:
		static ReceiverReport* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit ReceiverReport(Header* header);
		explicit ReceiverReport(ReceiverReport* report);

		// Locally generated Report. Holds the data internally.
		ReceiverReport();
		~ReceiverReport();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();
		const uint8_t* GetRaw();
		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		uint8_t GetFractionLost();
		void SetFractionLost(uint8_t fraction_lost);
		int32_t GetTotalLost();
		void SetTotalLost(int32_t total_lost);
		uint32_t GetLastSeq();
		void SetLastSeq(uint32_t last_seq);
		uint32_t GetJitter();
		void SetJitter(uint32_t jitter);
		uint32_t GetLastSenderReport();
		void SetLastSenderReport(uint32_t lsr);
		uint32_t GetDelaySinceLastSenderReport();
		void SetDelaySinceLastSenderReport(uint32_t dlsr);

	private:
		Header* header = nullptr;
		uint8_t* raw = nullptr;
	};

	class ReceiverReportPacket
		: public Packet
	{
	public:
		typedef std::vector<ReceiverReport*>::iterator Iterator;

	public:
		static ReceiverReportPacket* Parse(const uint8_t* data, size_t len, size_t offset=0);

	public:
		ReceiverReportPacket();
		~ReceiverReportPacket();

		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		void AddReport(ReceiverReport* report);
		Iterator Begin();
		Iterator End();

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* data) override;
		virtual size_t GetCount() override;
		virtual size_t GetSize() override;

	private:
		// SSRC of packet sender.
		uint32_t ssrc;
		std::vector<ReceiverReport*> reports;
	};

	/* Inline instance methods. */

	inline
	size_t ReceiverReport::GetSize()
	{
		return sizeof(Header);
	}

	inline
	const uint8_t* ReceiverReport::GetRaw()
	{
		return this->raw;
	}

	inline
	uint32_t ReceiverReport::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	void ReceiverReport::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	uint8_t ReceiverReport::GetFractionLost()
	{
		return (uint8_t)Utils::Byte::Get1Byte((uint8_t*)this->header, 4);
	}

	inline
	void ReceiverReport::SetFractionLost(uint8_t fraction_lost)
	{
		Utils::Byte::Set1Byte((uint8_t*)this->header, 4, fraction_lost);
	}

	inline
	int32_t ReceiverReport::GetTotalLost()
	{
		uint32_t value = (uint32_t)Utils::Byte::Get3Bytes((uint8_t*)this->header, 5);

		// Possitive value.
		if (((value >> 23) & 1) == 0)
			return value;

		// Negative value.
		else if (value != 0x0800000)
			value &= ~(1 << 23);

		return -value;
	}

	inline
	void ReceiverReport::SetTotalLost(int32_t total_lost)
	{
		// Get the limit value for possitive and negative total_lost.
		int32_t clamp = (total_lost >= 0) ?
			total_lost > 0x07FFFFF? 0x07FFFFF : total_lost :
			-total_lost > 0x0800000? 0x0800000 : -total_lost;

		uint32_t value = (total_lost >= 0) ?
			(clamp & 0x07FFFFF) : (clamp | 0x0800000);

		Utils::Byte::Set3Bytes((uint8_t*)this->header, 5, value);
	}

	inline
	uint32_t ReceiverReport::GetLastSeq()
	{
		return (uint32_t)ntohl(this->header->last_seq);
	}

	inline
	void ReceiverReport::SetLastSeq(uint32_t last_seq)
	{
		this->header->last_seq = (uint32_t)htonl(last_seq);
	}

	inline
	uint32_t ReceiverReport::GetJitter()
	{
		return (uint32_t)ntohl(this->header->jitter);
	}

	inline
	void ReceiverReport::SetJitter(uint32_t jitter)
	{
		this->header->jitter = (uint32_t)htonl(jitter);
	}

	inline
	uint32_t ReceiverReport::GetLastSenderReport()
	{
		return (uint32_t)ntohl(this->header->lsr);
	}

	inline
	void ReceiverReport::SetLastSenderReport(uint32_t lsr)
	{
		this->header->lsr = (uint32_t)htonl(lsr);
	}

	inline
	uint32_t ReceiverReport::GetDelaySinceLastSenderReport()
	{
		return (uint32_t)ntohl(this->header->dlsr);
	}

	inline
	void ReceiverReport::SetDelaySinceLastSenderReport(uint32_t dlsr)
	{
		this->header->dlsr = (uint32_t)htonl(dlsr);
	}

	/* Inline instance methods. */

	inline
	ReceiverReport::ReceiverReport()
	{
		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*)this->raw;
	}

	inline
	ReceiverReport::ReceiverReport(Header* header) :
		header(header)
	{}

	inline
	ReceiverReport::ReceiverReport(ReceiverReport* report) :
		header(report->header)
	{}

	inline
	ReceiverReport::~ReceiverReport()
	{
		if (this->raw)
			delete this->raw;
	}

	inline
	ReceiverReportPacket::ReceiverReportPacket()
		: Packet(Type::RR)
	{}

	inline
	ReceiverReportPacket::~ReceiverReportPacket()
	{
		for (auto report : this->reports)
		{
			delete report;
		}
	}

	inline
	size_t ReceiverReportPacket::GetCount()
	{
		return this->reports.size();
	}

	inline
	size_t ReceiverReportPacket::GetSize()
	{
		size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc);

		for (auto report : reports)
		{
			size += report->GetSize();
		}

		return size;
	}

	inline
	uint32_t ReceiverReportPacket::GetSsrc()
	{
		return (uint32_t)ntohl(this->ssrc);
	}

	inline
	void ReceiverReportPacket::SetSsrc(uint32_t ssrc)
	{
		this->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	void ReceiverReportPacket::AddReport(ReceiverReport* report)
	{
		this->reports.push_back(report);
	}

	inline
	ReceiverReportPacket::Iterator ReceiverReportPacket::Begin()
	{
		return this->reports.begin();
	}

	inline
	ReceiverReportPacket::Iterator ReceiverReportPacket::End()
	{
		return this->reports.end();
	}
}}

#endif
