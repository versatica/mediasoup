#ifndef MS_RTC_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class CompoundPacket
		{
		public:
			CompoundPacket() = default;

		public:
			const uint8_t* GetData() const;
			size_t GetSize() const;
			size_t GetSenderReportCount() const;
			size_t GetReceiverReportCount() const;
			void Dump() const;
			void AddSenderReport(SenderReport* report);
			void AddReceiverReport(ReceiverReport* report);
			void AddSdesChunk(SdesChunk* chunk);
			void Serialize(uint8_t* data);

		private:
			uint8_t* header{ nullptr };
			size_t size{ 0 };
			SenderReportPacket senderReportPacket;
			ReceiverReportPacket receiverReportPacket;
			SdesPacket sdesPacket;
		};

		/* Inline methods. */

		inline const uint8_t* CompoundPacket::GetData() const
		{
			return this->header;
		}

		inline size_t CompoundPacket::GetSize() const
		{
			return this->size;
		}

		inline size_t CompoundPacket::GetSenderReportCount() const
		{
			return this->senderReportPacket.GetCount();
		}

		inline size_t CompoundPacket::GetReceiverReportCount() const
		{
			return this->receiverReportPacket.GetCount();
		}

		inline void CompoundPacket::AddReceiverReport(ReceiverReport* report)
		{
			this->receiverReportPacket.AddReport(report);
		}

		inline void CompoundPacket::AddSdesChunk(SdesChunk* chunk)
		{
			this->sdesPacket.AddChunk(chunk);
		}
	} // namespace RTCP
} // namespace RTC

#endif
