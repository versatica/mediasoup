#define MS_CLASS "RTC::RTCP::ReceiverExtendedReport"
// #define MS_LOG_DEV

#include "RTC/RTCP/ReceiverExtendedReport.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		ReceiverExtendedReport::BlockHeader* ReceiverExtendedReport::ParseBlockHeader(
		  const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the header.
			auto* header = const_cast<BlockHeader*>(reinterpret_cast<const BlockHeader*>(data));

			// Packet size must be >= header size.
			if (sizeof(BlockHeader) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for receiver extended report, packet discarded");

				return nullptr;
			}

			return header;
		}

		ReceiverExtendedReport* ReceiverExtendedReport::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the body.
			auto* body = const_cast<BlockBody*>(reinterpret_cast<const BlockBody*>(data));

			// Packet size must be >= body size.
			if (sizeof(BlockBody) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for receiver extended report, packet discarded");

				return nullptr;
			}

			return new ReceiverExtendedReport(body);
		}

		/* Instance methods. */

		void ReceiverExtendedReport::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ReceiverExtendedReport>");
			MS_DUMP("  ntp sec      : %" PRIu32, GetNtpSec());
			MS_DUMP("  ntp frac     : %" PRIu32, GetNtpFrac());
			MS_DUMP("</ReceiverExtendedReport>");
		}

		size_t ReceiverExtendedReport::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			// Copy the body.
			std::memcpy(buffer, this->body, sizeof(BlockBody));

			return sizeof(BlockBody);
		}

		/* Class methods. */

		ReceiverExtendedReportPacket* ReceiverExtendedReportPacket::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			// Get the body.
			auto* body = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

			// Ensure there is space for the common body and the SSRC of packet receiver.
			if (sizeof(CommonHeader) + sizeof(uint32_t) > len)
			{
				MS_WARN_TAG(rtcp, "not enough space for receiver extended report packet, packet discarded");

				return nullptr;
			}

			std::unique_ptr<ReceiverExtendedReportPacket> packet(new ReceiverExtendedReportPacket());

			packet->SetSsrc(Utils::Byte::Get4Bytes(reinterpret_cast<uint8_t*>(body), sizeof(CommonHeader)));

			auto offset = sizeof(Packet::CommonHeader) + sizeof(uint32_t) /* ssrc */;

			while (len > offset)
			{
				// Get the header.
				auto header = ReceiverExtendedReport::ParseBlockHeader(data + offset, len - offset);
				if (header != nullptr)
				{
					offset += sizeof(ReceiverExtendedReport::BlockHeader) /* header */;
					if (header->blockType == 4)
					{
						// Get the body.
						ReceiverExtendedReport* report =
						  ReceiverExtendedReport::Parse(data + offset, len - offset);
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

		size_t ReceiverExtendedReportPacket::Serialize(uint8_t* buffer)
		{
			MS_TRACE();

			size_t offset = Packet::Serialize(buffer);

			// Copy the SSRC.
			std::memcpy(buffer + sizeof(Packet::CommonHeader), &this->ssrc, sizeof(this->ssrc));
			offset += sizeof(this->ssrc);

			// Serialize header.
			if (this->GetCount())
			{
				std::memcpy(buffer + offset, this->header, sizeof(ReceiverExtendedReport::BlockHeader));
				offset += sizeof(ReceiverExtendedReport::BlockHeader);
			}

			// Serialize report.
			if (this->report != nullptr)
			{
				offset += this->report->Serialize(buffer + offset);
			}

			return offset;
		}

		void ReceiverExtendedReportPacket::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ReceiverExtendedReportPacket>");
			MS_DUMP("  ssrc: %" PRIu32, static_cast<uint32_t>(ntohl(this->ssrc)));
			if (this->report != nullptr)
			{
				this->report->Dump();
			}
			MS_DUMP("</ReceiverExtendedReportPacket>");
		}
	} // namespace RTCP
} // namespace RTC
