#ifndef MS_RTC_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace RTCP
	{
		class CompoundPacket
		{
		public:
			/**
			 * What a stream being sent reports in a compound packet. They must go
			 * together in the same compound packet.
			 */
			struct SenderReportItems
			{
				/**
				 * Never null.
				 */
				std::unique_ptr<SenderReport> senderReport;
				/**
				 * Never null.
				 */
				std::unique_ptr<SdesChunk> sdesChunk;
				/**
				 * May be null.
				 */
				std::unique_ptr<DelaySinceLastRr::SsrcInfo> delaySinceLastRrSsrcInfo;
			};

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
			void Dump(int indentation = 0);
			/**
			 * Add what a stream being sent reports.
			 *
			 * @return `true` if they fit in the packet, in which case the packet takes
			 *   them and `items` is left empty. `false` otherwise, in which case
			 *   neither the packet nor `items` are modified.
			 */
			[[nodiscard]] bool AddSenderReportItems(SenderReportItems& items);
			/**
			 * Add the report of a stream being received, along with a Receiver
			 * Reference Time report with given NTP time if the packet has none yet.
			 *
			 * @return `true` if they fit in the packet, in which case the packet takes
			 *   the report and `report` is left empty. `false` otherwise, in which case
			 *   neither the packet nor `report` are modified.
			 */
			[[nodiscard]] bool AddReceiverReport(
			  std::unique_ptr<ReceiverReport>& report, const Utils::Time::Ntp& ntp);
			bool HasSenderReport()
			{
				return this->senderReportPacket.Begin() != this->senderReportPacket.End();
			}
			bool HasReceiverReferenceTime()
			{
				return std::ranges::any_of(
				  this->xrPacket.Begin(),
				  this->xrPacket.End(),
				  [](const ExtendedReportBlock* report) -> bool
				  {
					  return report->GetType() == ExtendedReportBlock::Type::RRT;
				  });
			}
			bool HasDelaySinceLastRr()
			{
				return std::ranges::any_of(
				  this->xrPacket.Begin(),
				  this->xrPacket.End(),
				  [](const ExtendedReportBlock* report) -> bool
				  {
					  return report->GetType() == ExtendedReportBlock::Type::DLRR;
				  });
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
