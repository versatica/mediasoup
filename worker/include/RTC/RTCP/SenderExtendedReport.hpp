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
			struct Header
			{
				uint8_t blockType;
				uint8_t reserved;
				uint16_t blockLength;
				uint32_t ssrc;
				uint32_t lrr;
				uint32_t dlrr;
			};

		public:
			static SenderExtendedReport* Parse(const uint8_t* data, size_t len);

		public:
			explicit SenderExtendedReport(Header* header);
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
			Header* header{ nullptr };
			uint8_t raw[sizeof(Header)]{ 0 };
		};

		class SenderExtendedReportPacket : public Packet
		{
		public:
			using Iterator = std::vector<SenderExtendedReport*>::iterator;

		public:
			static SenderExtendedReportPacket* Parse(const uint8_t* data, size_t len, size_t offset = 0);

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
			std::vector<SenderExtendedReport*> reports;
		};

		inline SenderExtendedReport::SenderExtendedReport()
		{
			this->header              = reinterpret_cast<Header*>(this->raw);
			this->header->blockType   = 5;
			this->header->reserved    = 0;
			this->header->blockLength = uint16_t{ ntohs(3) };
		}

		inline SenderExtendedReport::SenderExtendedReport(Header* header) : header(header)
		{
		}

		inline SenderExtendedReport::SenderExtendedReport(SenderExtendedReport* report)
		  : header(report->header)
		{
		}

		inline size_t SenderExtendedReport::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint32_t SenderExtendedReport::GetSsrc() const
		{
			return uint32_t{ ntohl(this->header->ssrc) };
		}

		inline void SenderExtendedReport::SetSsrc(uint32_t ssrc)
		{
			this->header->ssrc = uint32_t{ htonl(ssrc) };
		}

		inline uint32_t SenderExtendedReport::GetLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->header->lrr) };
		}

		inline void SenderExtendedReport::SetLastReceiverReport(uint32_t lrr)
		{
			this->header->lrr = uint32_t{ htonl(lrr) };
		}

		inline uint32_t SenderExtendedReport::GetDelaySinceLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->header->dlrr) };
		}

		inline void SenderExtendedReport::SetDelaySinceLastReceiverReport(uint32_t dlrr)
		{
			this->header->dlrr = uint32_t{ htonl(dlrr) };
		}

		inline SenderExtendedReportPacket::SenderExtendedReportPacket() : Packet(Type::XR)
		{
		}

		inline SenderExtendedReportPacket::~SenderExtendedReportPacket()
		{
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
			size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc);

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
			this->reports.clear();
			this->reports.push_back(report);
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
