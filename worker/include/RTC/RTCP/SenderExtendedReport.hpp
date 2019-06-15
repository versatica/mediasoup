#ifndef MS_RTC_RTCP_SENDER_EXTENDED_REPORT_HPP
#define MS_RTC_RTCP_SENDER_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class SenderExtendedReport
		{
		public:
			struct BlockHeader
			{
				uint8_t blockType;
				uint8_t reserved;
				uint16_t blockLength;
			};

		public:
			struct BlockBody
			{
				uint32_t ssrc;
				uint32_t lrr;
				uint32_t dlrr;
			};

		public:
			static BlockHeader* ParseBlockHeader(const uint8_t* data, size_t len);
			static SenderExtendedReport* Parse(const uint8_t* data, size_t len);

		public:
			explicit SenderExtendedReport(BlockBody* body);
			explicit SenderExtendedReport(SenderExtendedReport* report);

			SenderExtendedReport();

			void Dump() const;
			size_t Serialize(uint8_t* buffer);
			size_t GetSize() const;
			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			uint32_t GetLastReceiverReport() const;
			void SetLastReceiverReport(uint32_t lrr);
			uint32_t GetDelaySinceLastReceiverReport() const;
			void SetDelaySinceLastReceiverReport(uint32_t dlrr);

		private:
			BlockBody* body{ nullptr };
			uint8_t raw[sizeof(BlockBody)]{ 0 };
		};

		class SenderExtendedReportPacket : public Packet
		{
		public:
			using Iterator = std::vector<SenderExtendedReport*>::iterator;

		public:
			static SenderExtendedReportPacket* Parse(const uint8_t* data, size_t len);

		public:
			SenderExtendedReportPacket();
			~SenderExtendedReportPacket() override;

			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			void AddReport(SenderExtendedReport* report);
			Iterator Begin();
			Iterator End();

		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetCount() const override;
			size_t GetSize() const override;

		private:
			uint32_t ssrc{ 0 };
			SenderExtendedReport::BlockHeader* header;
			std::vector<SenderExtendedReport*> reports;
		};

		inline SenderExtendedReport::SenderExtendedReport()
		{
			this->body = reinterpret_cast<BlockBody*>(this->raw);
		}

		inline SenderExtendedReport::SenderExtendedReport(BlockBody* body) : body(body)
		{
		}

		inline SenderExtendedReport::SenderExtendedReport(SenderExtendedReport* report)
		  : body(report->body)
		{
		}

		inline size_t SenderExtendedReport::GetSize() const
		{
			return sizeof(BlockBody);
		}

		inline uint32_t SenderExtendedReport::GetSsrc() const
		{
			return uint32_t{ ntohl(this->body->ssrc) };
		}

		inline void SenderExtendedReport::SetSsrc(uint32_t ssrc)
		{
			this->body->ssrc = uint32_t{ htonl(ssrc) };
		}

		inline uint32_t SenderExtendedReport::GetLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->body->lrr) };
		}

		inline void SenderExtendedReport::SetLastReceiverReport(uint32_t lrr)
		{
			this->body->lrr = uint32_t{ htonl(lrr) };
		}

		inline uint32_t SenderExtendedReport::GetDelaySinceLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->body->dlrr) };
		}

		inline void SenderExtendedReport::SetDelaySinceLastReceiverReport(uint32_t dlrr)
		{
			this->body->dlrr = uint32_t{ htonl(dlrr) };
		}

		inline SenderExtendedReportPacket::SenderExtendedReportPacket() : Packet(Type::XR)
		{
			this->header = new SenderExtendedReport::BlockHeader{ 5, 0, 0 };
		}

		inline SenderExtendedReportPacket::~SenderExtendedReportPacket()
		{
			delete this->header;
			for (auto* report : this->reports)
			{
				delete report;
			}
		}

		inline size_t SenderExtendedReportPacket::GetCount() const
		{
			return this->reports.size();
		}

		inline size_t SenderExtendedReportPacket::GetSize() const
		{
			size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc) +
			              sizeof(SenderExtendedReport::BlockHeader);

			for (auto* report : this->reports)
			{
				size += report->GetSize();
			}

			return size;
		}

		inline uint32_t SenderExtendedReportPacket::GetSsrc() const
		{
			return uint32_t{ ntohl(this->ssrc) };
		}

		inline void SenderExtendedReportPacket::SetSsrc(uint32_t ssrc)
		{
			this->ssrc = uint32_t{ htonl(ssrc) };
		}

		inline void SenderExtendedReportPacket::AddReport(SenderExtendedReport* report)
		{
			this->reports.push_back(report);
			this->header->blockLength = uint16_t{ ntohs(this->reports.size() * 3) };
		}

		inline SenderExtendedReportPacket::Iterator SenderExtendedReportPacket::Begin()
		{
			return this->reports.begin();
		}

		inline SenderExtendedReportPacket::Iterator SenderExtendedReportPacket::End()
		{
			return this->reports.end();
		}
	} // namespace RTCP
} // namespace RTC

#endif
