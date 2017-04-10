#ifndef MS_RTC_RTCP_SENDER_REPORT_HPP
#define MS_RTC_RTCP_SENDER_REPORT_HPP

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class SenderReport
		{
		public:
			/* Struct for RTCP sender report. */
			struct Header
			{
				uint32_t ssrc;
				uint32_t ntpSec;
				uint32_t ntpFrac;
				uint32_t rtpTs;
				uint32_t packetCount;
				uint32_t octetCount;
			};

		public:
			static SenderReport* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			explicit SenderReport(Header* header);
			explicit SenderReport(SenderReport* report);

			// Locally generated Report. Holds the data internally.
			SenderReport();

			void Dump() const;
			size_t Serialize(uint8_t* buffer);
			size_t GetSize() const;
			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			uint32_t GetNtpSec() const;
			void SetNtpSec(uint32_t ntpSec);
			uint32_t GetNtpFrac() const;
			void SetNtpFrac(uint32_t ntpFrac);
			uint32_t GetRtpTs() const;
			void SetRtpTs(uint32_t rtpTs);
			uint32_t GetPacketCount() const;
			void SetPacketCount(uint32_t packetCount);
			uint32_t GetOctetCount() const;
			void SetOctetCount(uint32_t octetCount);

		private:
			Header* header              = nullptr;
			uint8_t raw[sizeof(Header)] = {0};
		};

		class SenderReportPacket : public Packet
		{
		public:
			typedef std::vector<SenderReport*>::iterator Iterator;

		public:
			static SenderReportPacket* Parse(const uint8_t* data, size_t len);

		public:
			SenderReportPacket();
			virtual ~SenderReportPacket();

			void AddReport(SenderReport* report);
			Iterator Begin();
			Iterator End();

			/* Pure virtual methods inherited from Packet. */
		public:
			virtual void Dump() const override;
			virtual size_t Serialize(uint8_t* buffer) override;
			virtual size_t GetCount() const override;
			virtual size_t GetSize() const override;

		private:
			std::vector<SenderReport*> reports;
		};

		/* Inline instance methods. */

		inline SenderReport::SenderReport()
		{
			this->header = reinterpret_cast<Header*>(this->raw);
		}

		inline SenderReport::SenderReport(Header* header) : header(header)
		{
		}

		inline SenderReport::SenderReport(SenderReport* report) : header(report->header)
		{
		}

		inline size_t SenderReport::GetSize() const
		{
			return sizeof(Header);
		}

		inline uint32_t SenderReport::GetSsrc() const
		{
			return (uint32_t)ntohl(this->header->ssrc);
		}

		inline void SenderReport::SetSsrc(uint32_t ssrc)
		{
			this->header->ssrc = (uint32_t)htonl(ssrc);
		}

		inline uint32_t SenderReport::GetNtpSec() const
		{
			return (uint32_t)ntohl(this->header->ntpSec);
		}

		inline void SenderReport::SetNtpSec(uint32_t ntpSec)
		{
			this->header->ntpSec = (uint32_t)htonl(ntpSec);
		}

		inline uint32_t SenderReport::GetNtpFrac() const
		{
			return (uint32_t)ntohl(this->header->ntpFrac);
		}

		inline void SenderReport::SetNtpFrac(uint32_t ntpFrac)
		{
			this->header->ntpFrac = (uint32_t)htonl(ntpFrac);
		}

		inline uint32_t SenderReport::GetRtpTs() const
		{
			return (uint32_t)ntohl(this->header->rtpTs);
		}

		inline void SenderReport::SetRtpTs(uint32_t rtpTs)
		{
			this->header->rtpTs = (uint32_t)htonl(rtpTs);
		}

		inline

		    uint32_t
		    SenderReport::GetPacketCount() const
		{
			return (uint32_t)ntohl(this->header->packetCount);
		}

		inline void SenderReport::SetPacketCount(uint32_t packetCount)
		{
			this->header->packetCount = (uint32_t)htonl(packetCount);
		}

		inline uint32_t SenderReport::GetOctetCount() const
		{
			return (uint32_t)ntohl(this->header->octetCount);
		}

		inline void SenderReport::SetOctetCount(uint32_t octetCount)
		{
			this->header->octetCount = (uint32_t)htonl(octetCount);
		}

		/* Inline instance methods. */

		inline SenderReportPacket::SenderReportPacket() : Packet(Type::SR)
		{
		}

		inline SenderReportPacket::~SenderReportPacket()
		{
			for (auto report : this->reports)
			{
				delete report;
			}
		}

		inline size_t SenderReportPacket::GetCount() const
		{
			return this->reports.size();
		}

		inline size_t SenderReportPacket::GetSize() const
		{
			size_t size = sizeof(Packet::CommonHeader);

			for (auto report : this->reports)
			{
				size += report->GetSize();
			}

			return size;
		}

		inline void SenderReportPacket::AddReport(SenderReport* report)
		{
			this->reports.push_back(report);
		}

		inline SenderReportPacket::Iterator SenderReportPacket::Begin()
		{
			return this->reports.begin();
		}

		inline SenderReportPacket::Iterator SenderReportPacket::End()
		{
			return this->reports.end();
		}
	}
}

#endif
