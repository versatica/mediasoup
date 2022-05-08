#ifndef MS_RTC_RTCP_BYE_HPP
#define MS_RTC_RTCP_BYE_HPP

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <string>
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		class ByePacket : public Packet
		{
		public:
			using Iterator = std::vector<uint32_t>::iterator;

		public:
			static ByePacket* Parse(const uint8_t* data, size_t len);

		public:
			ByePacket() : Packet(Type::BYE)
			{
			}
			explicit ByePacket(CommonHeader* commonHeader) : Packet(commonHeader)
			{
			}
			~ByePacket() override = default;

			void AddSsrc(uint32_t ssrc)
			{
				this->ssrcs.push_back(ssrc);
			}
			void SetReason(const std::string& reason)
			{
				this->reason = reason;
			}
			const std::string& GetReason() const
			{
				return this->reason;
			}
			Iterator Begin()
			{
				return this->ssrcs.begin();
			}
			Iterator End()
			{
				return this->ssrcs.end();
			}

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetCount() const override
			{
				return this->ssrcs.size();
			}
			size_t GetSize() const override
			{
				size_t size = Packet::CommonHeaderSize;

				size += ssrcs.size() * 4u;

				if (!this->reason.empty())
				{
					size += 1u; // Length field.
					size += this->reason.length();
				}

				// http://stackoverflow.com/questions/11642210/computing-padding-required-for-n-byte-alignment
				// Consider pading to 32 bits (4 bytes) boundary.
				return (size + 3) & ~3;
			}

		private:
			std::vector<uint32_t> ssrcs;
			std::string reason;
		};
	} // namespace RTCP
} // namespace RTC

#endif
