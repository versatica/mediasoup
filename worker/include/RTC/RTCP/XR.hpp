#ifndef MS_RTC_RTCP_XR_HPP
#define MS_RTC_RTCP_XR_HPP

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <vector>

/* https://tools.ietf.org/html/rfc3611
 * RTP Control Protocol Extended Reports (RTCP XR)

    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|reserved |   PT=XR=207   |             length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              SSRC                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                         report blocks                         :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */

namespace RTC
{
	namespace RTCP
	{
		class ExtendedReportBlock
		{
		public:
			enum class Type : uint8_t
			{
				LRLE = 1,
				DRLE = 2,
				PRT  = 3,
				RRT  = 4,
				DLRR = 5,
				SS   = 6,
				VM   = 7
			};

		public:
			/* Struct for Extended Report Block common header. */
			struct CommonHeader
			{
				uint8_t blockType : 8;
				uint8_t reserved : 8;
				uint16_t length : 16;
			};

		public:
			static ExtendedReportBlock* Parse(const uint8_t* data, size_t len);

		public:
			ExtendedReportBlock(Type type);
			virtual ~ExtendedReportBlock() = default;

			ExtendedReportBlock::Type GetType() const;

		public:
			virtual void Dump() const                 = 0;
			virtual size_t Serialize(uint8_t* buffer) = 0;
			virtual size_t GetSize() const            = 0;

		protected:
			Type type;
			CommonHeader* header{ nullptr };

		private:
			uint8_t raw[sizeof(CommonHeader)] = { 0 };
		};

		class ExtendedReportPacket : public Packet
		{
		public:
			using Iterator = std::vector<ExtendedReportBlock*>::iterator;

		public:
			static ExtendedReportPacket* Parse(const uint8_t* data, size_t len);

		public:
			ExtendedReportPacket();
			~ExtendedReportPacket() override = default;

			void AddReport(ExtendedReportBlock* report);
			uint32_t GetSsrc() const;
			void SetSsrc(uint32_t ssrc);
			Iterator Begin();
			Iterator End();

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetCount() const override;
			size_t GetSize() const override;

		private:
			uint32_t ssrc{ 0 };
			std::vector<ExtendedReportBlock*> reports;
		};

		// Inline ExtendedReportBlock methods.

		inline ExtendedReportBlock::ExtendedReportBlock(Type type) : type(type)
		{
			this->header = reinterpret_cast<CommonHeader*>(this->raw);

			this->header->reserved = 0;
		}

		inline ExtendedReportBlock::Type ExtendedReportBlock::GetType() const
		{
			return this->type;
		}

		inline void ExtendedReportPacket::AddReport(ExtendedReportBlock* report)
		{
			this->reports.push_back(report);
		}

		inline uint32_t ExtendedReportPacket::GetSsrc() const
		{
			return uint32_t{ ntohl(this->ssrc) };
		}

		inline void ExtendedReportPacket::SetSsrc(uint32_t ssrc)
		{
			this->ssrc = uint32_t{ htonl(ssrc) };
		}

		// Inline ExtendedReportPacket methods.

		inline ExtendedReportPacket::ExtendedReportPacket() : Packet(Type::XR)
		{
		}

		inline ExtendedReportPacket::Iterator ExtendedReportPacket::Begin()
		{
			return this->reports.begin();
		}

		inline ExtendedReportPacket::Iterator ExtendedReportPacket::End()
		{
			return this->reports.end();
		}

		inline size_t ExtendedReportPacket::GetCount() const
		{
			return 0;
		}

		inline size_t ExtendedReportPacket::GetSize() const
		{
			size_t size = sizeof(Packet::CommonHeader) + sizeof(this->ssrc);

			for (auto* report : this->reports)
			{
				size += report->GetSize();
			}

			return size;
		}
	} // namespace RTCP
} // namespace RTC

#endif
