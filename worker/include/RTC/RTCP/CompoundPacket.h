#ifndef MS_RTC_RTCP_COMPOUND_PACKET_H
#define MS_RTC_RTCP_COMPOUND_PACKET_H

#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/Sdes.h"
#include "Logger.h" // MS_ASSERT

#include <vector>

namespace RTC
{
namespace RTCP
{
	class CompoundPacket
	{
	public:
		CompoundPacket(){};
		~CompoundPacket();

		uint8_t* GetRaw();
		size_t GetSize();
		size_t GetSenderReportCount();
		size_t GetReceiverReportCount();

		void Dump();
		void AddSenderReport(SenderReport* report);
		void AddReceiverReport(ReceiverReport* report);
		void AddSdesChunk(SdesChunk* chunk);
		void Serialize();

	private:
		uint8_t* raw = nullptr;
		size_t size = 0;

		SenderReportPacket senderReportPacket;
		ReceiverReportPacket receiverReportPacket;
		SdesPacket sdesPacket;
	};

	/* Inline methods. */

	inline
	uint8_t* CompoundPacket::GetRaw()
	{
		return this->raw;
	}

	inline
	size_t CompoundPacket::GetSize()
	{
		return this->size;
	}

	inline
	size_t CompoundPacket::GetSenderReportCount()
	{
		return this->senderReportPacket.GetCount();
	}

	inline
	size_t CompoundPacket::GetReceiverReportCount()
	{
		return this->receiverReportPacket.GetCount();
	}

	inline
	void CompoundPacket::AddSenderReport(SenderReport* report)
	{
		MS_ASSERT(this->senderReportPacket.GetCount() == 0, "A sender report is already present");

		this->senderReportPacket.AddReport(report);
	}

	inline
	void CompoundPacket::AddReceiverReport(ReceiverReport* report)
	{
		this->receiverReportPacket.AddReport(report);
	}

	inline
	void CompoundPacket::AddSdesChunk(SdesChunk* chunk)
	{
		this->sdesPacket.AddChunk(chunk);
	}
}
}

#endif
