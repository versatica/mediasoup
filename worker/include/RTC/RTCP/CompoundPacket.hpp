#ifndef MS_RTC_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class CompoundPacket
		{
		public:
			using UniquePtr = std::unique_ptr<CompoundPacket>;
			static UniquePtr Create();

		public:
			const uint8_t* GetData() const
			{
				return this->header;
			}
			size_t GetSize() const
			{
				return this->size;
			}
			size_t GetSenderReportCount() const
			{
				return this->senderReportPacket.GetCount();
			}
			size_t GetReceiverReportCount() const
			{
				return this->receiverReportPacket.GetCount();
			}
			void Dump();
			void AddSenderReport(SenderReport* report);
			void AddReceiverReport(ReceiverReport* report);
			void AddSdesChunk(SdesChunk* chunk);
			void AddReceiverReferenceTime(ReceiverReferenceTime* report);
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
			void Serialize(uint8_t* data);

		private:
			// Use `CompoundPacket::Create()` instead
			CompoundPacket() = default;
			// Use `CompoundPacket::ReturnIntoPool()` instead
			~CompoundPacket() = default;

			friend struct std::default_delete<RTC::RTCP::CompoundPacket>;
			static void ReturnIntoPool(CompoundPacket* packet);

		private:
			uint8_t* header{ nullptr };
			size_t size{ 0 };
			SenderReportPacket senderReportPacket;
			ReceiverReportPacket receiverReportPacket;
			SdesPacket sdesPacket;
			ExtendedReportPacket xrPacket;
		};
	} // namespace RTCP
} // namespace RTC

namespace std
{
	template<>
	struct default_delete<RTC::RTCP::CompoundPacket>
	{
		void operator()(RTC::RTCP::CompoundPacket* ptr) const
		{
			RTC::RTCP::CompoundPacket::ReturnIntoPool(ptr);
		}
	};
}; // namespace std
#endif
