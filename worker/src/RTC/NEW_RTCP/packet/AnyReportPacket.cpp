#define MS_CLASS "RTC::NEW_RTCP::AnyReportPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/AnyReportPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Instance methods. */

		AnyReportPacket::AnyReportPacket(uint8_t* buffer, size_t bufferLength)
		  : Packet(buffer, bufferLength)
		{
			MS_TRACE();
		}

		void AnyReportPacket::SetSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, ssrc);
		}

		std::vector<AnyReportPacket::ReportBlock> AnyReportPacket::GetReportBlocks() const
		{
			MS_TRACE();

			const auto* const reportBlocksPointer = GetReportBlocksPointer();
			const uint8_t numberOfReportBlocks    = GetNumberOfReportBlocks();

			std::vector<ReportBlock> reportBlocks;

			reportBlocks.reserve(numberOfReportBlocks);

			for (uint8_t idx{ 0 }; idx < numberOfReportBlocks; ++idx)
			{
				const auto* const ptr = reportBlocksPointer + (idx * AnyReportPacket::ReportBlockLength);

				reportBlocks.push_back(
				  { .ssrc                       = Utils::Byte::Get4Bytes(ptr, 0),
					  .fractionLost               = Utils::Byte::Get1Byte(ptr, 4),
					  .totalLost                  = Utils::Byte::Get3BytesSigned(ptr, 5),
					  .extendedHighestSeq         = Utils::Byte::Get4Bytes(ptr, 8),
					  .jitter                     = Utils::Byte::Get4Bytes(ptr, 12),
					  .lastSenderReport           = Utils::Byte::Get4Bytes(ptr, 16),
					  .delaySinceLastSenderReport = Utils::Byte::Get4Bytes(ptr, 20) });
			}

			return reportBlocks;
		}

		void AnyReportPacket::AddReportBlock(const ReportBlock& reportBlock)
		{
			MS_TRACE();

			if (GetNumberOfReportBlocks() == AnyReportPacket::MaxReportBlocks)
			{
				MS_THROW_TYPE_ERROR(
				  "cannot add more than %zu report blocks", AnyReportPacket::MaxReportBlocks);
			}

			if (HasProfileSpecificExtensions())
			{
				MS_THROW_TYPE_ERROR("cannot add a report block to a packet with profile-specific extensions");
			}

			// NOTE: This may throw.
			SetVariableLengthValueLength(
			  GetVariableLengthValueLength() + AnyReportPacket::ReportBlockLength);

			// The cumulative number of packets lost is a 24 bits signed integer, so
			// a value out of its range is clamped as stated in the appendix A.3 of
			// RFC 3550.
			const int32_t totalLost = std::clamp(reportBlock.totalLost, -(1 << 23), (1 << 23) - 1);

			// The new report block goes right after the last one (the RC field has
			// not been incremented yet).
			auto* const ptr = GetProfileSpecificExtensionsPointer();

			// Write the report block.
			Utils::Byte::Set4Bytes(ptr, 0, reportBlock.ssrc);
			Utils::Byte::Set1Byte(ptr, 4, reportBlock.fractionLost);
			Utils::Byte::Set3BytesSigned(ptr, 5, totalLost);
			Utils::Byte::Set4Bytes(ptr, 8, reportBlock.extendedHighestSeq);
			Utils::Byte::Set4Bytes(ptr, 12, reportBlock.jitter);
			Utils::Byte::Set4Bytes(ptr, 16, reportBlock.lastSenderReport);
			Utils::Byte::Set4Bytes(ptr, 20, reportBlock.delaySinceLastSenderReport);

			// Update the counter field.
			SetCount(GetCount() + 1);
		}

		bool AnyReportPacket::IsLengthValid() const
		{
			MS_TRACE();

			if (GetProfileSpecificExtensionsPointer() > GetBuffer() + GetLength())
			{
				MS_WARN_TAG(
				  rtcp,
				  "fixed header and %" PRIu8 " report blocks exceed the packet length (%zu bytes)",
				  GetNumberOfReportBlocks(),
				  GetLength());

				return false;
			}

			return true;
		}

		void AnyReportPacket::DumpReportBlocks(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "  report blocks: %" PRIu8, GetNumberOfReportBlocks());
			for (const auto& reportBlock : GetReportBlocks())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - ssrc: %" PRIu32 ", fraction lost: %" PRIu8 ", total lost: %" PRIi32
				  ", extended highest seq: %" PRIu32 ", jitter: %" PRIu32 ", lsr: %" PRIu32
				  ", dlsr: %" PRIu32,
				  reportBlock.ssrc,
				  reportBlock.fractionLost,
				  reportBlock.totalLost,
				  reportBlock.extendedHighestSeq,
				  reportBlock.jitter,
				  reportBlock.lastSenderReport,
				  reportBlock.delaySinceLastSenderReport);
			}
		}

		bool AnyReportPacket::HasProfileSpecificExtensions() const
		{
			MS_TRACE();

			return (GetBuffer() + GetLength()) > GetProfileSpecificExtensionsPointer();
		}
	} // namespace NEW_RTCP
} // namespace RTC
