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
			struct BlockHeader
			{
				uint8_t blockType;
				uint8_t reserved;
				uint16_t blockLength;
			};

		public:
			struct BlockBody
			{
				uint32_t ntpSec;
				uint32_t ntpFrac;
			};

		public:
			static BlockHeader* ParseBlockHeader(const uint8_t* data, size_t len);
			static ReceiverExtendedReport* Parse(const uint8_t* data, size_t len);

		public:
			explicit ReceiverExtendedReport(BlockBody* body);
			explicit ReceiverExtendedReport(ReceiverExtendedReport* report);

			ReceiverExtendedReport();

			void Dump() const;
			size_t Serialize(uint8_t* buffer);
			size_t GetSize() const;
			uint32_t GetNtpSec() const;
			void SetNtpSec(uint32_t ntpSec);
			uint32_t GetNtpFrac() const;
			void SetNtpFrac(uint32_t ntpFrac);

		private:
			BlockBody* body{ nullptr };
			uint8_t raw[sizeof(BlockBody)]{ 0 };
		};

		class ReceiverExtendedReportPacket : public Packet
		{
		public:
			static ReceiverExtendedReportPacket* Parse(const uint8_t* data, size_t len);

		public:
			ReceiverExtendedReportPacket();
			~ReceiverExtendedReportPacket() override;

			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			void AddReport(ReceiverExtendedReport* report);

		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetCount() const override;
			size_t GetSize() const override;

		private:
			uint32_t ssrc{ 0 };
			ReceiverExtendedReport::BlockHeader* header;
			ReceiverExtendedReport* report;
		};

		inline ReceiverExtendedReport::ReceiverExtendedReport()
		{
			this->body = reinterpret_cast<BlockBody*>(this->raw);
		}

		inline ReceiverExtendedReport::ReceiverExtendedReport(BlockBody* body) : body(body)
		{
		}

		inline ReceiverExtendedReport::ReceiverExtendedReport(ReceiverExtendedReport* report)
		  : body(report->body)
		{
		}

		inline size_t ReceiverExtendedReport::GetSize() const
		{
			return sizeof(BlockBody);
		}

		inline uint32_t ReceiverExtendedReport::GetNtpSec() const
		{
			return uint32_t{ ntohl(this->body->ntpSec) };
		}

		inline void ReceiverExtendedReport::SetNtpSec(uint32_t ntpSec)
		{
			this->body->ntpSec = uint32_t{ htonl(ntpSec) };
		}

		inline uint32_t ReceiverExtendedReport::GetNtpFrac() const
		{
			return uint32_t{ ntohl(this->body->ntpFrac) };
		}

		inline void ReceiverExtendedReport::SetNtpFrac(uint32_t ntpFrac)
		{
			this->body->ntpFrac = uint32_t{ htonl(ntpFrac) };
		}

		inline ReceiverExtendedReportPacket::ReceiverExtendedReportPacket() : Packet(Type::XR)
		{
			this->header = new ReceiverExtendedReport::BlockHeader{ 4, 0, uint16_t{ ntohs(2) } };
		}

		inline ReceiverExtendedReportPacket::~ReceiverExtendedReportPacket()
		{
			delete this->header;
			delete this->report;
		}

		inline size_t ReceiverExtendedReportPacket::GetCount() const
		{
			if (this->report != nullptr)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}

		inline size_t ReceiverExtendedReportPacket::GetSize() const
		{
			size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc) +
			              sizeof(ReceiverExtendedReport::BlockHeader);

			if (this->report != nullptr)
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
			delete this->report;
			this->report = report;
		}
	} // namespace RTCP
} // namespace RTC

#endif
