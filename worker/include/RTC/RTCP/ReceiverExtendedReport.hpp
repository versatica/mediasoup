#ifndef MS_RTC_RTCP_RECEIVER_EXTENDED_REPORT_HPP
#define MS_RTC_RTCP_RECEIVER_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class ReceiverExtendedReport
		{
		public:
			struct Header
			{
				uint8_t blockType;
				uint8_t reserved;
				uint16_t blockLength;
				uint32_t ntpSec;
				uint32_t ntpFrac;
			};

		public:
			static ReceiverExtendedReport* Parse(const uint8_t* data, size_t len);

		public:
			explicit ReceiverExtendedReport(Header* header);
			explicit ReceiverExtendedReport(ReceiverExtendedReport* report);

			ReceiverExtendedReport();

			void Dump() const;
			size_t Serialize(uint8_t* buffer);
			size_t GetSize() const;
			uint8_t GetBlockType() const;
			void SetBlockType(uint8_t blockType);
			uint8_t GetReserved() const;
			void SetReserved(uint8_t reserved);
			uint16_t GetBlockLength() const;
			void SetBlockLength(uint16_t blockLength);
			uint32_t GetNtpSec() const;
			void SetNtpSec(uint32_t ntpSec);
			uint32_t GetNtpFrac() const;
			void SetNtpFrac(uint32_t ntpFrac);

		private:
			Header* header{ nullptr };
			uint8_t raw[sizeof(Header)]{ 0 };
		};

		class ReceiverExtendedReportPacket : public Packet
		{
		public:
			using Iterator = std::vector<ReceiverExtendedReport*>::iterator;

		public:
			static ReceiverExtendedReportPacket* Parse(const uint8_t* data, size_t len, size_t offset = 0);

		public:
			ReceiverExtendedReportPacket();
			~ReceiverExtendedReportPacket() override;

			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			void AddReport(ReceiverExtendedReport* report);
			Iterator Begin();
			Iterator End();

		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetCount() const override;
			size_t GetSize() const override;

		private:
			uint32_t ssrc{ 0 };
			std::vector<ReceiverExtendedReport*> reports;
		};

		inline ReceiverExtendedReport::ReceiverExtendedReport()
		{
			this->header = reinterpret_cast<Header*>(this->raw);
		}

		inline ReceiverExtendedReport::ReceiverExtendedReport(Header* header) : header(header)
		{
		}

		inline ReceiverExtendedReport::ReceiverExtendedReport(ReceiverExtendedReport* report)
		  : header(report->header)
		{
		}

		inline size_t ReceiverExtendedReport::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint8_t ReceiverExtendedReport::GetBlockType() const
		{
			return this->header->blockType;
		}

		inline void ReceiverExtendedReport::SetBlockType(uint8_t blockType)
		{
			this->header->blockType = blockType;
		}

		inline uint8_t ReceiverExtendedReport::GetReserved() const
		{
			return this->header->reserved;
		}

		inline void ReceiverExtendedReport::SetReserved(uint8_t reserved)
		{
			this->header->reserved = reserved;
		}

		inline uint16_t ReceiverExtendedReport::GetBlockLength() const
		{
			return uint16_t{ ntohs(this->header->blockLength) };
		}

		inline void ReceiverExtendedReport::SetBlockLength(uint16_t blockLength)
		{
			this->header->blockLength = uint16_t{ ntohs(blockLength) };
		}

		inline uint32_t ReceiverExtendedReport::GetNtpSec() const
		{
			return uint32_t{ ntohl(this->header->ntpSec) };
		}

		inline void ReceiverExtendedReport::SetNtpSec(uint32_t ntpSec)
		{
			this->header->ntpSec = uint32_t{ htonl(ntpSec) };
		}

		inline uint32_t ReceiverExtendedReport::GetNtpFrac() const
		{
			return uint32_t{ ntohl(this->header->ntpFrac) };
		}

		inline void ReceiverExtendedReport::SetNtpFrac(uint32_t ntpFrac)
		{
			this->header->ntpFrac = uint32_t{ htonl(ntpFrac) };
		}

		inline ReceiverExtendedReportPacket::ReceiverExtendedReportPacket() : Packet(Type::XR)
		{
		}

		inline ReceiverExtendedReportPacket::~ReceiverExtendedReportPacket()
		{
			for (auto* report : this->reports)
			{
				delete report;
			}
		}

		inline size_t ReceiverExtendedReportPacket::GetCount() const
		{
			return this->reports.size();
		}

		inline size_t ReceiverExtendedReportPacket::GetSize() const
		{
			size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc);

			for (auto* report : this->reports)
			{
				size += report->GetSize();
			}

			return size;
		}

		inline uint32_t ReceiverExtendedReportPacket::GetSsrc() const
		{
			return uint32_t{ ntohl(this->ssrc) };
		}

		inline void ReceiverExtendedReportPacket::SetSsrc(uint32_t ssrc)
		{
			this->ssrc = uint32_t{ htonl(ssrc) };
		}

		inline void ReceiverExtendedReportPacket::AddReport(ReceiverExtendedReport* report)
		{
			this->reports.clear();
			this->reports.push_back(report);
		}

		inline ReceiverExtendedReportPacket::Iterator ReceiverExtendedReportPacket::Begin()
		{
			return this->reports.begin();
		}

		inline ReceiverExtendedReportPacket::Iterator ReceiverExtendedReportPacket::End()
		{
			return this->reports.end();
		}
	} // namespace RTCP
} // namespace RTC

#endif
