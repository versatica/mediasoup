#ifndef MS_RTC_RTCP_COMPOUND_PACKET_H
#define MS_RTC_RTCP_COMPOUND_PACKET_H

#include "common.h"
#include "RTC/RTCP/SenderReport.h"
#include "RTC/RTCP/ReceiverReport.h"
#include "RTC/RTCP/Sdes.h"
#include <vector>

namespace RTC { namespace RTCP
{
	class CompoundPacket
	{
	public:
		const uint8_t* GetData();
		size_t GetSize();
		size_t GetSenderReportCount();
		size_t GetReceiverReportCount();
		void Dump();
		void AddSenderReport(SenderReport* report);
		void AddReceiverReport(ReceiverReport* report);
		void AddSdesChunk(SdesChunk* chunk);
		void Serialize(uint8_t* buffer);

	private:
		uint8_t* header = nullptr;
		size_t size = 0;
		SenderReportPacket senderReportPacket;
		ReceiverReportPacket receiverReportPacket;
		SdesPacket sdesPacket;
	};

	/* Inline methods. */

	inline
	const uint8_t* CompoundPacket::GetData()
	{
		return this->header;
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
	void CompoundPacket::AddReceiverReport(ReceiverReport* report)
	{
		this->receiverReportPacket.AddReport(report);
	}

	inline
	void CompoundPacket::AddSdesChunk(SdesChunk* chunk)
	{
		this->sdesPacket.AddChunk(chunk);
	}
}}

#endif
