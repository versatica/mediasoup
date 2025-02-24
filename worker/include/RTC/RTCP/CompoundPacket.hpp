#ifndef MS_RTC_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
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
			const uint8_t* GetData() const
			{
				return this->header;
			}
			size_t GetSize();
			size_t GetSenderReportCount() const
			{
				return this->senderReportPacket.GetCount();
			}
			size_t GetReceiverReportCount() const
			{
				return this->receiverReportPacket.GetCount();
			}
			void Dump();
			// RTCP additions per Consumer (non pipe).
			// Adds the given data and returns true if there is enough space to hold it,
			// false otherwise.
			bool Add(
			  SenderReport* senderReport,
			  SdesChunk* sdesChunk,
			  DelaySinceLastRr::SsrcInfo* delaySinceLastRrSsrcInfo);
			// RTCP additions per Consumer (pipe).
			// Adds the given data and returns true if there is enough space to hold it,
			// false otherwise.
			bool Add(
			  std::vector<SenderReport*>& senderReports,
			  std::vector<SdesChunk*>& sdesChunks,
			  std::vector<DelaySinceLastRr::SsrcInfo*>& delaySinceLastRrSsrcInfos);
			// RTCP additions per Producer.
			// Adds the given data and returns true if there is enough space to hold it,
			// false otherwise.
			bool Add(std::vector<ReceiverReport*>&, ReceiverReferenceTime*);
			void AddSenderReport(SenderReport* report);
			void AddReceiverReport(ReceiverReport* report);
			void AddSdesChunk(SdesChunk* chunk);
			bool HasSenderReport()
			{
				return this->senderReportPacket.Begin() != this->senderReportPacket.End();
			}
			bool HasReceiverReferenceTime()
			{
				return std::any_of(
				  this->xrPacket.Begin(),
				  this->xrPacket.End(),
				  [](const ExtendedReportBlock* report)
				  { return report->GetType() == ExtendedReportBlock::Type::RRT; });
			}
			bool HasDelaySinceLastRr()
			{
				return std::any_of(
				  this->xrPacket.Begin(),
				  this->xrPacket.End(),
				  [](const ExtendedReportBlock* report)
				  { return report->GetType() == ExtendedReportBlock::Type::DLRR; });
			}
			void Serialize(uint8_t* data);

		private:
			uint8_t* header{ nullptr };
			SenderReportPacket senderReportPacket;
			ReceiverReportPacket receiverReportPacket;
			SdesPacket sdesPacket;
			ExtendedReportPacket xrPacket;
			DelaySinceLastRr* delaySinceLastRr{ nullptr };
		};
	} // namespace RTCP
} // namespace RTC

#endif
