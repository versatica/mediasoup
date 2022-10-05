#define MS_CLASS "RTC::RTCP::CompoundPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/CompoundPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Instance methods. */

		size_t CompoundPacket::GetSize()
		{
			size_t size{ 0 };

			if (this->senderReportPacket.GetCount() > 0u)
			{
				size += this->senderReportPacket.GetSize();
			}
			if (this->receiverReportPacket.GetCount() > 0u)
			{
				size += this->receiverReportPacket.GetSize();
			}

			if (this->sdesPacket.GetCount() > 0u)
				size += this->sdesPacket.GetSize();

			if (this->xrPacket.Begin() != this->xrPacket.End())
				size += this->xrPacket.GetSize();

			return size;
		}

		void CompoundPacket::Serialize(uint8_t* data)
		{
			MS_TRACE();

			this->header = data;

			// Fill it.
			size_t offset{ 0 };

			MS_ASSERT(
			  this->senderReportPacket.GetCount() > 0u || this->receiverReportPacket.GetCount() > 0u,
			  "no Sender or Receiver report present");

			if (this->senderReportPacket.GetCount() > 0u)
			{
				offset += this->senderReportPacket.Serialize(this->header);
			}

			if (this->receiverReportPacket.GetCount() > 0u)
			{
				offset += this->receiverReportPacket.Serialize(this->header + offset);
			}

			if (this->sdesPacket.GetCount() > 0u)
			{
				offset += this->sdesPacket.Serialize(this->header + offset);
			}

			if (this->xrPacket.Begin() != this->xrPacket.End())
			{
				offset += this->xrPacket.Serialize(this->header + offset);
			}
		}

		bool CompoundPacket::Add(
		  SenderReport* senderReport, SdesChunk* sdesChunk, DelaySinceLastRr* delaySinceLastRrReport)
		{
			// Add the items into the packet.

			if (senderReport)
				this->senderReportPacket.AddReport(senderReport);

			if (sdesChunk)
				this->sdesPacket.AddChunk(sdesChunk);

			if (delaySinceLastRrReport)
				this->xrPacket.AddReport(delaySinceLastRrReport);

			// New items can hold in the packet, report it.
			if (GetSize() <= MaxSize)
				return true;

			// New items can not hold in the packet, remove them,
			// delete and report it.

			if (senderReport)
			{
				this->senderReportPacket.RemoveReport(senderReport);
				delete senderReport;
			}

			if (sdesChunk)
			{
				this->sdesPacket.RemoveChunk(sdesChunk);
				delete sdesChunk;
			}

			if (delaySinceLastRrReport)
			{
				this->xrPacket.RemoveReport(delaySinceLastRrReport);
				delete delaySinceLastRrReport;
			}

			return false;
		}

		bool CompoundPacket::Add(
		  std::vector<SenderReport*>& senderReports,
		  std::vector<SdesChunk*>& sdesChunks,
		  std::vector<DelaySinceLastRr*>& delaySinceLastRrReports)
		{
			// Add the items into the packet.

			for (auto* report : senderReports)
				this->senderReportPacket.AddReport(report);

			for (auto* chunk : sdesChunks)
				this->sdesPacket.AddChunk(chunk);

			for (auto* report : delaySinceLastRrReports)
				this->xrPacket.AddReport(report);

			// New items can hold in the packet, report it.
			if (GetSize() <= MaxSize)
				return true;

			// New items can not hold in the packet, remove them,
			// delete and report it.

			for (auto* report : senderReports)
			{
				this->senderReportPacket.RemoveReport(report);
				delete report;
			}

			for (auto* chunk : sdesChunks)
			{
				this->sdesPacket.RemoveChunk(chunk);
				delete chunk;
			}

			for (auto* report : delaySinceLastRrReports)
			{
				this->xrPacket.RemoveReport(report);
				delete report;
			}

			return false;
		}

		bool CompoundPacket::Add(
		  std::vector<ReceiverReport*>& receiverReports,
		  ReceiverReferenceTime* receiverReferenceTimeReport)
		{
			// Add the items into the packet.

			for (auto* report : receiverReports)
				this->receiverReportPacket.AddReport(report);

			if (receiverReferenceTimeReport)
				this->xrPacket.AddReport(receiverReferenceTimeReport);

			// New items can hold in the packet, report it.
			if (GetSize() <= MaxSize)
				return true;

			// New items can not hold in the packet, remove them,
			// delete and report it.

			for (auto* report : receiverReports)
			{
				this->receiverReportPacket.RemoveReport(report);
				delete report;
			}

			if (receiverReferenceTimeReport)
			{
				this->xrPacket.RemoveReport(receiverReferenceTimeReport);
				delete receiverReferenceTimeReport;
			}

			return false;
		}

		void CompoundPacket::Dump()
		{
			MS_TRACE();

			MS_DUMP("<CompoundPacket>");

			if (HasSenderReport())
			{
				this->senderReportPacket.Dump();

				if (this->receiverReportPacket.GetCount() != 0u)
					this->receiverReportPacket.Dump();
			}
			else
			{
				this->receiverReportPacket.Dump();
			}

			if (this->sdesPacket.GetCount() != 0u)
				this->sdesPacket.Dump();

			if (this->xrPacket.Begin() != this->xrPacket.End())
				this->xrPacket.Dump();

			MS_DUMP("</CompoundPacket>");
		}

		void CompoundPacket::AddSenderReport(SenderReport* report)
		{
			MS_TRACE();

			this->senderReportPacket.AddReport(report);
		}

		void CompoundPacket::AddReceiverReport(ReceiverReport* report)
		{
			MS_TRACE();

			this->receiverReportPacket.AddReport(report);
		}

		void CompoundPacket::AddSdesChunk(SdesChunk* chunk)
		{
			MS_TRACE();

			this->sdesPacket.AddChunk(chunk);
		}

		void CompoundPacket::AddReceiverReferenceTime(ReceiverReferenceTime* report)
		{
			MS_TRACE();

			this->xrPacket.AddReport(report);
		}

		void CompoundPacket::AddDelaySinceLastRr(DelaySinceLastRr* report)
		{
			MS_TRACE();

			this->xrPacket.AddReport(report);
		}
	} // namespace RTCP
} // namespace RTC
