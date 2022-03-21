#ifndef MS_RTC_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
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
			struct CompoundPacketDeleter
			{
				void operator()(CompoundPacket* packet) const;
			};

			using UniquePtr       = std::unique_ptr<CompoundPacket, CompoundPacketDeleter>;
			using Allocator       = Utils::ObjectPoolAllocator<CompoundPacket>;
			using AllocatorTraits = std::allocator_traits<Allocator>;
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
			// Used by CompoundPacketDeleter
			~CompoundPacket() = default;

			friend struct CompoundPacketDeleter;
			friend AllocatorTraits;

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
	struct allocator_traits<RTC::RTCP::CompoundPacket::Allocator>
	{
		template<typename... Args>
		static void construct(
		  RTC::RTCP::CompoundPacket::Allocator& a, RTC::RTCP::CompoundPacket* p, Args&&... args)
		{
			new (p) RTC::RTCP::CompoundPacket(forward<Args>(args)...);
		}

		static void destroy(RTC::RTCP::CompoundPacket::Allocator& a, RTC::RTCP::CompoundPacket* p)
		{
			p->~CompoundPacket();
		}
	};
}; // namespace std
#endif
