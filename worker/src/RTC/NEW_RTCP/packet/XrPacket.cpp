#define MS_CLASS "RTC::NEW_RTCP::XrPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/XrPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/UnknownExtendedReport.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		XrPacket* XrPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::XR)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return XrPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		XrPacket* XrPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < XrPacket::XrHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new XrPacket(buffer, bufferLength);

			packet->InitializeHeader(Packet::PacketType::XR, 0, XrPacket::XrHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSsrc(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum XrPacket length.

			return packet;
		}

		size_t XrPacket::ComputeLengthFor(size_t extendedReportsLength)
		{
			MS_TRACE();

			// The Extended Reports go right after the XR header, and each one is a
			// multiple of 4 bytes.
			return XrPacket::XrHeaderLength + extendedReportsLength;
		}

		XrPacket* XrPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < XrPacket::XrHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp, "XrPacket length must be equal or greater than %zu", XrPacket::XrHeaderLength);

				return nullptr;
			}

			auto* const packet = new XrPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			// Pointer that initially points to the first Extended Report block (if
			// any) and is later incremented to point to each following one.
			const auto* ptr       = buffer + XrPacket::XrHeaderLength;
			const auto* const end = buffer + contentLength;

			// Extended Report blocks must fill the whole packet (excluding padding),
			// otherwise the last IsExtendedReport() call fails.
			while (ptr < end)
			{
				// The remaining length in the packet (excluding padding) is the
				// potential buffer length of the Extended Report block.
				const size_t extendedReportMaxBufferLength = end - ptr;

				// Here we must anticipate the type of each Extended Report block to use
				// its appropriate parser.
				ExtendedReport::BlockType blockType;
				size_t extendedReportLength;

				if (!ExtendedReport::IsExtendedReport(
				      ptr, extendedReportMaxBufferLength, blockType, extendedReportLength))
				{
					MS_WARN_TAG(rtcp, "not an Extended Report block");

					delete packet;
					return nullptr;
				}

				ExtendedReport* extendedReport{ nullptr }; // NOLINT(misc-const-correctness)

				switch (blockType)
				{
					case ExtendedReport::BlockType::RRT:
					{
						extendedReport =
						  RrtExtendedReport::ParseStrict(ptr, extendedReportLength, extendedReportLength);

						break;
					}

					case ExtendedReport::BlockType::DLRR:
					{
						extendedReport =
						  DlrrExtendedReport::ParseStrict(ptr, extendedReportLength, extendedReportLength);

						break;
					}

					default:
					{
						extendedReport =
						  UnknownExtendedReport::ParseStrict(ptr, extendedReportLength, extendedReportLength);
					}
				}

				if (!extendedReport)
				{
					delete packet;
					return nullptr;
				}

				packet->extendedReports.push_back(extendedReport);

				ptr += extendedReport->GetLength();
			}

			return packet;
		}

		/* Instance methods. */

		XrPacket::XrPacket(uint8_t* buffer, size_t bufferLength) : Packet(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(XrPacket::XrHeaderLength);
		}

		XrPacket::~XrPacket()
		{
			MS_TRACE();

			for (const auto* extendedReport : this->extendedReports)
			{
				delete extendedReport;
			}
		}

		void XrPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::XrPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			MS_DUMP_CLEAN(indentation, "  extended reports count: %zu", GetExtendedReportsCount());
			MS_DUMP_CLEAN(
			  indentation,
			  "  needs consolidation of extended reports: %s",
			  NeedsConsolidation() ? "yes" : "no");
			for (const auto* extendedReport : this->extendedReports)
			{
				extendedReport->Dump(indentation + 1);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::XrPacket>");
		}

		void XrPacket::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* const previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Packet::Serialize(buffer, bufferLength);

			for (auto* extendedReport : this->extendedReports)
			{
				const size_t offset = extendedReport->GetBuffer() - previousBuffer;

				extendedReport->SoftSerialize(buffer + offset);
			}
		}

		XrPacket* XrPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new XrPacket(buffer, bufferLength);

			CloneInto(clonedPacket);
			SoftCloneInto(clonedPacket);

			return clonedPacket;
		}

		void XrPacket::SetSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, ssrc);
		}

		void XrPacket::AddExtendedReport(const ExtendedReport* extendedReport)
		{
			MS_TRACE();

			AssertDoesNotNeedConsolidation();

			const size_t previousValueLength = GetVariableLengthValueLength();

			// This will remove the padding (if any) and update the total length and
			// length field of the packet.
			// NOTE: It may throw.
			SetVariableLengthValueLength(previousValueLength + extendedReport->GetLength());

			// Let's append the Extended Report block at the end of existing ones.
			auto* const clonedExtendedReport = extendedReport->Clone(
			  GetVariableLengthValuePointer() + previousValueLength, extendedReport->GetLength());

			// Add the Extended Report block to the list.
			this->extendedReports.push_back(clonedExtendedReport);
		}

		void XrPacket::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			const auto* const previousBuffer = GetBuffer();

			Packet::SoftSerialize(buffer);

			for (auto* extendedReport : this->extendedReports)
			{
				const size_t offset = extendedReport->GetBuffer() - previousBuffer;

				extendedReport->SoftSerialize(buffer + offset);
			}
		}

		XrPacket* XrPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new XrPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}

		void XrPacket::SoftCloneInto(Packet* packet) const
		{
			MS_TRACE();

			auto* const xrPacket = static_cast<XrPacket*>(packet);

			// Soft clone Extended Report blocks into the given packet.
			for (auto* extendedReport : this->extendedReports)
			{
				const size_t offset = extendedReport->GetBuffer() - GetBuffer();

				auto* const softClonedExtendedReport =
				  extendedReport->SoftClone(xrPacket->GetBuffer() + offset);

				xrPacket->extendedReports.push_back(softClonedExtendedReport);
			}

			Packet::SoftCloneInto(packet);
		}

		void XrPacket::HandleInPlaceExtendedReport(ExtendedReport* extendedReport)
		{
			MS_TRACE();

			SetNeedsConsolidation(true);

			// When the application completes the Extended Report block it must call
			// `extendedReport->Consolidate()` or `extendedReport->Discard()` and that
			// will trigger this event.
			extendedReport->SetBuildInPlaceListener(
			  [this, extendedReport](bool consolidate) -> void
			  {
				  if (!consolidate)
				  {
					  SetNeedsConsolidation(false);

					  delete extendedReport;

					  return;
				  }

				  try
				  {
					  // Fix buffer length assigned to the Extended Report block.
					  extendedReport->SetBufferLength(extendedReport->GetLength());

					  // This will update the total length and length field of the packet.
					  // NOTE: It may throw.
					  SetVariableLengthValueLength(GetVariableLengthValueLength() + extendedReport->GetLength());

					  // Add the Extended Report block to the list.
					  this->extendedReports.push_back(extendedReport);

					  SetNeedsConsolidation(false);
				  }
				  catch (const MediaSoupError&)
				  {
					  SetNeedsConsolidation(false);

					  throw;
				  }
			  });
		}
	} // namespace NEW_RTCP
} // namespace RTC
