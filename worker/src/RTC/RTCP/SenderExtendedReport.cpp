#define MS_CLASS "RTC::RTCP::SenderExtendedReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/SenderExtendedReport.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		SenderExtendedReport::BlockHeader* SenderExtendedReport::ParseBlockHeader(
		  const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<BlockHeader*>(reinterpret_cast<const BlockHeader*>(data));

			// Packet size must be >= header size.
			if (sizeof(BlockHeader) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for sender extended report, packet discarded");

				return nullptr;
			}

			return header;
		}

		SenderExtendedReport* SenderExtendedReport::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the body.
			auto* body = const_cast<BlockBody*>(reinterpret_cast<const BlockBody*>(data));

			// Packet size must be >= body size.
			if (sizeof(BlockBody) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for sender extended report, packet discarded");

				return nullptr;
			}

			return new SenderExtendedReport(body);
		}

		/* Instance methods. */

		void SenderExtendedReport::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SenderExtendedReport>");
			MS_DUMP("  ssrc         : %" PRIu32, GetSsrc());
			MS_DUMP("  lrr          : %" PRIu32, GetLastReceiverReport());
			MS_DUMP("  dlrr         : %" PRIu32, GetDelaySinceLastReceiverReport());
			MS_DUMP("</SenderExtendedReport>");
		}

		size_t SenderExtendedReport::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Copy the body.
			std::memcpy(buffer, this->body, sizeof(BlockBody));

			return sizeof(BlockBody);
		}

		/* Class methods. */

		SenderExtendedReportPacket* SenderExtendedReportPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the body.
			auto* body = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common body and the SSRC of packet sender.
			if (sizeof(CommonHeader) + sizeof(uint32_t) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for sender extended report packet, packet discarded");

				return nullptr;
			}

			std::unique_ptr<SenderExtendedReportPacket> packet(new SenderExtendedReportPacket());

			packet->SetSsrc(Utils::Byte::Get4Bytes(reinterpret_cast<uint8_t*>(body), sizeof(CommonHeader)));

			auto offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

			while (len > offset)
			{
				// Get the header.
				auto header = SenderExtendedReport::ParseBlockHeader(data + offset, len - offset);
				if (header != nullptr)
				{
					offset += sizeof(SenderExtendedReport::BlockHeader) /* header */;
					if (header->blockType == 5)
					{
						for (int i = 0; i < uint16_t{ ntohs(header->blockLength) } / 3; i++)
						{
							// Get the body.
							SenderExtendedReport* report = SenderExtendedReport::Parse(data + offset, len - offset);
							if (report != nullptr && len > offset)
							{
								packet->AddReport(report);
								offset += uint16_t{ ntohs(header->blockLength) } * 32 /* body */;
							}
							else
							{
								return packet.release();
							}
						}
					}
					else
					{
						offset += uint16_t{ ntohs(header->blockLength) } * 32 /* body */;
					}
				}
				else
				{
					return packet.release();
				}
			}

			return packet.release();
		}

		/* Instance methods. */

		size_t SenderExtendedReportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			size_t offset = Packet::Serialize(buffer);

			// Copy the SSRC.
			std::memcpy(buffer + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
			offset += sizeof(this->ssrc);

			// Serialize header.
			if (this->GetCount())
			{
				std::memcpy(buffer + offset, this->header, sizeof(SenderExtendedReport::BlockHeader));
				offset += sizeof(SenderExtendedReport::BlockHeader);
			}

			// Serialize reports.
			for (auto* report : this->reports)
			{
				offset += report->Serialize(buffer + offset);
			}

			return offset;
		}

		void SenderExtendedReportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<SenderExtendedReportPacket>");
			MS_DUMP("  ssrc: %" PRIu32, static_cast<uint32_t>(ntohl(this->ssrc)));
			for (auto* report : this->reports)
			{
				report->Dump();
			}
			MS_DUMP("</SenderExtendedReportPacket>");
		}
	} // namespace RTCP
} // namespace RTC
