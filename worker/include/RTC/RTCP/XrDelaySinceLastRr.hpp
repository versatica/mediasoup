#ifndef MS_RTC_RTCP_XR_DELAY_SINCE_LAST_RR_HPP
#define MS_RTC_RTCP_XR_DELAY_SINCE_LAST_RR_HPP

#include "common.hpp"
#include "RTC/RTCP/XR.hpp"

/* https://tools.ietf.org/html/rfc3611
 * Delay Since Last Receiver Report (DLRR) Report Block

 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |     BT=5      |   reserved    |         block length          |
 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 |                 SSRC_1 (SSRC of first receiver)               | sub-
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
 |                         last RR (LRR)                         |   1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                   delay since last RR (DLRR)                  |
 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 |                 SSRC_2 (SSRC of second receiver)              | sub-
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
 :                               ...                             :   2
 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */

namespace RTC
{
	namespace RTCP
	{
		class DelaySinceLastRr : public ExtendedReportBlock
		{
		public:
			class SsrcInfo
			{
			public:
				struct Body
				{
					uint32_t ssrc;
					uint32_t lrr;
					uint32_t dlrr;
				};

			public:
				static SsrcInfo* Parse(const uint8_t* data, size_t len);

			public:
				// Parsed Report. Points to an external data.
				explicit SsrcInfo(Body* body);

				// Locally generated Report. Holds the data internally.
				SsrcInfo();

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
				Body* body{ nullptr };
				uint8_t raw[sizeof(Body)] = { 0 };
			};

		public:
			using Iterator = std::vector<SsrcInfo*>::iterator;

		public:
			static DelaySinceLastRr* Parse(const uint8_t* data, size_t len);

		public:
			explicit DelaySinceLastRr(CommonHeader* header);
			DelaySinceLastRr();
			~DelaySinceLastRr();

			Iterator Begin();
			Iterator End();

		public:
			void AddSsrcInfo(SsrcInfo* ssrcInfo);

			/* Pure virtual methods inherited from ExtendedReportBlock. */
		public:
			virtual void Dump() const override;
			virtual size_t Serialize(uint8_t* buffer) override;
			virtual size_t GetSize() const override;

		private:
			std::vector<SsrcInfo*> ssrcInfos;
		};

		/* Inline SsrcInfo instance methods. */

		inline DelaySinceLastRr::SsrcInfo::SsrcInfo()
		{
			this->body = reinterpret_cast<Body*>(this->raw);
		}

		inline DelaySinceLastRr::SsrcInfo::SsrcInfo(Body* body) : body(body)
		{
		}

		inline size_t DelaySinceLastRr::SsrcInfo::GetSize() const
		{
			return sizeof(Body);
		}

		inline uint32_t DelaySinceLastRr::SsrcInfo::GetSsrc() const
		{
			return uint32_t{ ntohl(this->body->ssrc) };
		}

		inline void DelaySinceLastRr::SsrcInfo::SetSsrc(uint32_t ssrc)
		{
			this->body->ssrc = uint32_t{ htonl(ssrc) };
		}

		inline uint32_t DelaySinceLastRr::SsrcInfo::GetLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->body->lrr) };
		}

		inline void DelaySinceLastRr::SsrcInfo::SetLastReceiverReport(uint32_t lrr)
		{
			this->body->lrr = uint32_t{ htonl(lrr) };
		}

		inline uint32_t DelaySinceLastRr::SsrcInfo::GetDelaySinceLastReceiverReport() const
		{
			return uint32_t{ ntohl(this->body->dlrr) };
		}

		inline void DelaySinceLastRr::SsrcInfo::SetDelaySinceLastReceiverReport(uint32_t dlrr)
		{
			this->body->dlrr = uint32_t{ htonl(dlrr) };
		}

		/* Inline DelaySinceLastRr instance methods. */

		inline DelaySinceLastRr::DelaySinceLastRr(CommonHeader* header)
		  : ExtendedReportBlock(ExtendedReportBlock::Type::DLRR)
		{
			this->header = header;
		}

		inline DelaySinceLastRr::DelaySinceLastRr()
		  : ExtendedReportBlock(ExtendedReportBlock::Type::DLRR)
		{
		}

		inline DelaySinceLastRr::~DelaySinceLastRr()
		{
			for (auto* ssrcInfo : this->ssrcInfos)
			{
				delete ssrcInfo;
			}
		}

		inline void DelaySinceLastRr::AddSsrcInfo(SsrcInfo* ssrcInfo)
		{
			this->ssrcInfos.push_back(ssrcInfo);
		}

		inline DelaySinceLastRr::Iterator DelaySinceLastRr::Begin()
		{
			return this->ssrcInfos.begin();
		}

		inline DelaySinceLastRr::Iterator DelaySinceLastRr::End()
		{
			return this->ssrcInfos.end();
		}

		inline size_t DelaySinceLastRr::GetSize() const
		{
			size_t size{ 4 }; // Common header.

			for (auto* ssrcInfo : this->ssrcInfos)
			{
				size += ssrcInfo->GetSize();
			}

			return size;
		}
	} // namespace RTCP
} // namespace RTC

#endif
