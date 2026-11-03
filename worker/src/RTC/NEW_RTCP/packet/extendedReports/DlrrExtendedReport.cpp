#define MS_CLASS "RTC::NEW_RTCP::DlrrExtendedReport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		DlrrExtendedReport* DlrrExtendedReport::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ExtendedReport::BlockType blockType;
			size_t extendedReportLength;

			if (!ExtendedReport::IsExtendedReport(buffer, bufferLength, blockType, extendedReportLength))
			{
				return nullptr;
			}

			if (blockType != ExtendedReport::BlockType::DLRR)
			{
				MS_WARN_DEV("invalid block type");

				return nullptr;
			}

			return DlrrExtendedReport::ParseStrict(buffer, bufferLength, extendedReportLength);
		}

		DlrrExtendedReport* DlrrExtendedReport::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ExtendedReport::ExtendedReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const extendedReport = new DlrrExtendedReport(buffer, bufferLength);

			extendedReport->InitializeHeader(
			  ExtendedReport::BlockType::DLRR, 0, ExtendedReport::ExtendedReportHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum DlrrExtendedReport length.

			return extendedReport;
		}

		DlrrExtendedReport* DlrrExtendedReport::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength)
		{
			MS_TRACE();

			const size_t subBlocksLength =
			  extendedReportLength - ExtendedReport::ExtendedReportHeaderLength;

			// The block must contain whole sub-blocks.
			if (subBlocksLength % DlrrExtendedReport::SubBlockLength != 0)
			{
				MS_WARN_TAG(
				  rtcp,
				  "sub-blocks length (%zu bytes) is not multiple of %zu",
				  subBlocksLength,
				  DlrrExtendedReport::SubBlockLength);

				return nullptr;
			}

			auto* const extendedReport = new DlrrExtendedReport(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			extendedReport->SetLength(extendedReportLength);

			return extendedReport;
		}

		/* Instance methods. */

		DlrrExtendedReport::DlrrExtendedReport(uint8_t* buffer, size_t bufferLength)
		  : ExtendedReport(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ExtendedReport::ExtendedReportHeaderLength);
		}

		DlrrExtendedReport::~DlrrExtendedReport()
		{
			MS_TRACE();
		}

		void DlrrExtendedReport::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::DlrrExtendedReport>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  sub-blocks: %zu", GetNumberOfSubBlocks());
			for (const auto& subBlock : GetSubBlocks())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  - ssrc: %" PRIu32 ", lrr: %" PRIu32 ", dlrr: %" PRIu32,
				  subBlock.ssrc,
				  subBlock.lastReceiverReport,
				  subBlock.delaySinceLastReceiverReport);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::DlrrExtendedReport>");
		}

		DlrrExtendedReport* DlrrExtendedReport::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedExtendedReport = new DlrrExtendedReport(buffer, bufferLength);

			CloneInto(clonedExtendedReport);

			return clonedExtendedReport;
		}

		std::vector<DlrrExtendedReport::SubBlock> DlrrExtendedReport::GetSubBlocks() const
		{
			MS_TRACE();

			const auto* const subBlocksPointer = GetVariableLengthValuePointer();
			const size_t numberOfSubBlocks     = GetNumberOfSubBlocks();

			std::vector<SubBlock> subBlocks;

			subBlocks.reserve(numberOfSubBlocks);

			for (size_t idx{ 0 }; idx < numberOfSubBlocks; ++idx)
			{
				const auto* const ptr = subBlocksPointer + (idx * DlrrExtendedReport::SubBlockLength);

				subBlocks.push_back(
				  { .ssrc                         = Utils::Byte::Get4Bytes(ptr, 0),
					  .lastReceiverReport           = Utils::Byte::Get4Bytes(ptr, 4),
					  .delaySinceLastReceiverReport = Utils::Byte::Get4Bytes(ptr, 8) });
			}

			return subBlocks;
		}

		void DlrrExtendedReport::AddSubBlock(const SubBlock& subBlock)
		{
			MS_TRACE();

			// The new sub-block goes right after the last one.
			auto* const ptr = GetVariableLengthValuePointer() + GetVariableLengthValueLength();

			// NOTE: This may throw.
			SetVariableLengthValueLength(
			  GetVariableLengthValueLength() + DlrrExtendedReport::SubBlockLength);

			// Write the sub-block.
			Utils::Byte::Set4Bytes(ptr, 0, subBlock.ssrc);
			Utils::Byte::Set4Bytes(ptr, 4, subBlock.lastReceiverReport);
			Utils::Byte::Set4Bytes(ptr, 8, subBlock.delaySinceLastReceiverReport);
		}

		DlrrExtendedReport* DlrrExtendedReport::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedExtendedReport =
			  new DlrrExtendedReport(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedExtendedReport);

			return softClonedExtendedReport;
		}
	} // namespace NEW_RTCP
} // namespace RTC
