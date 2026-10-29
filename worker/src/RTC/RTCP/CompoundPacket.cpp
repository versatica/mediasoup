#define MS_CLASS "RTC::RTCP::CompoundPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/CompoundPacket.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Instance methods. */

		size_t CompoundPacket::GetSize()
		{
			MS_TRACE();

			size_t size{ 0 };

			if (this->senderReportPacket.GetCount() > 0)
			{
				size += this->senderReportPacket.GetSize();
			}

			if (this->receiverReportPacket.GetCount() > 0)
			{
				size += this->receiverReportPacket.GetSize();
			}

			if (this->sdesPacket.GetCount() > 0)
			{
				size += this->sdesPacket.GetSize();
			}

			if (this->xrPacket.Begin() != this->xrPacket.End())
			{
				size += this->xrPacket.GetSize();
			}

			return size;
		}

		void CompoundPacket::Serialize(uint8_t* data)
		{
			MS_TRACE();

			this->header = data;

			// Fill it.
			size_t offset{ 0 };

			MS_ASSERT(
			  this->senderReportPacket.GetCount() > 0 || this->receiverReportPacket.GetCount() > 0,
			  "no Sender or Receiver report present");

			if (this->senderReportPacket.GetCount() > 0)
			{
				offset += this->senderReportPacket.Serialize(this->header);
			}

			if (this->receiverReportPacket.GetCount() > 0)
			{
				offset += this->receiverReportPacket.Serialize(this->header + offset);
			}

			if (this->sdesPacket.GetCount() > 0)
			{
				offset += this->sdesPacket.Serialize(this->header + offset);
			}

			if (this->xrPacket.Begin() != this->xrPacket.End())
			{
				this->xrPacket.Serialize(this->header + offset);
			}
		}

		bool CompoundPacket::AddSendStreamItems(SendStreamItems& items)
		{
			MS_TRACE();

			MS_ASSERT(items.senderReport, "no Sender Report given");
			MS_ASSERT(items.sdesChunk, "no SDES chunk given");

			auto* const senderReport             = items.senderReport.release();
			auto* const sdesChunk                = items.sdesChunk.release();
			auto* const delaySinceLastRrSsrcInfo = items.delaySinceLastRrSsrcInfo.release();

			bool delaySinceLastRrCreated{ false };

			// Add the items into the packet.

			this->senderReportPacket.AddReport(senderReport);
			this->sdesPacket.AddChunk(sdesChunk);

			if (delaySinceLastRrSsrcInfo)
			{
				// Add a DLRR block into the XR packet if no present.
				if (!this->delaySinceLastRr)
				{
					this->delaySinceLastRr = new RTC::RTCP::DelaySinceLastRr();
					this->xrPacket.AddReport(this->delaySinceLastRr);

					delaySinceLastRrCreated = true;
				}

				this->delaySinceLastRr->AddSsrcInfo(delaySinceLastRrSsrcInfo);
			}

			// The items fit in the packet, so it keeps them.
			if (GetSize() <= RTC::Consts::OutgoingRtcpPacketMaxSize)
			{
				return true;
			}

			// The items do not fit in the packet, so take them out of it and give
			// them back to the caller.

			this->senderReportPacket.RemoveReport(senderReport);
			this->sdesPacket.RemoveChunk(sdesChunk);

			if (delaySinceLastRrSsrcInfo)
			{
				this->delaySinceLastRr->RemoveSsrcInfo(delaySinceLastRrSsrcInfo);

				// Also remove the DLRR block if it was created for them.
				if (delaySinceLastRrCreated)
				{
					this->xrPacket.RemoveReport(this->delaySinceLastRr);

					delete this->delaySinceLastRr;

					this->delaySinceLastRr = nullptr;
				}
			}

			items.senderReport.reset(senderReport);
			items.sdesChunk.reset(sdesChunk);
			items.delaySinceLastRrSsrcInfo.reset(delaySinceLastRrSsrcInfo);

			return false;
		}

		bool CompoundPacket::AddReceiverReport(
		  std::unique_ptr<ReceiverReport>& report, const Utils::Time::Ntp& ntp)
		{
			MS_TRACE();

			MS_ASSERT(report, "no Receiver Report given");

			auto* const receiverReport = report.release();

			ReceiverReferenceTime* receiverReferenceTime{ nullptr };

			// Add the items into the packet.

			this->receiverReportPacket.AddReport(receiverReport);

			// Add a Receiver Reference Time report if not present.
			if (!HasReceiverReferenceTime())
			{
				receiverReferenceTime = new ReceiverReferenceTime();

				receiverReferenceTime->SetNtpSec(ntp.seconds);
				receiverReferenceTime->SetNtpFrac(ntp.fractions);

				this->xrPacket.AddReport(receiverReferenceTime);
			}

			// The items fit in the packet, so it keeps them.
			if (GetSize() <= RTC::Consts::OutgoingRtcpPacketMaxSize)
			{
				return true;
			}

			// The items do not fit in the packet, so take them out of it and give
			// the report back to the caller.

			this->receiverReportPacket.RemoveReport(receiverReport);

			if (receiverReferenceTime)
			{
				this->xrPacket.RemoveReport(receiverReferenceTime);

				delete receiverReferenceTime;
			}

			report.reset(receiverReport);

			return false;
		}

		void CompoundPacket::Dump(int indentation)
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<CompoundPacket>");

			if (HasSenderReport())
			{
				this->senderReportPacket.Dump(indentation + 1);

				if (this->receiverReportPacket.GetCount() != 0)
				{
					this->receiverReportPacket.Dump(indentation + 1);
				}
			}
			else
			{
				this->receiverReportPacket.Dump(indentation + 1);
			}

			if (this->sdesPacket.GetCount() != 0)
			{
				this->sdesPacket.Dump(indentation + 1);
			}

			if (this->xrPacket.Begin() != this->xrPacket.End())
			{
				this->xrPacket.Dump(indentation + 1);
			}

			MS_DUMP_CLEAN(indentation, "</CompoundPacket>");
		}
	} // namespace RTCP
} // namespace RTC
