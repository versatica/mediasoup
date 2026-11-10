#define MS_CLASS "RTC::RTCP::UnknownExtendedReport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/extendedReports/UnknownExtendedReport.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		UnknownExtendedReport* UnknownExtendedReport::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ExtendedReport::BlockType blockType;
			size_t extendedReportLength;

			if (!ExtendedReport::IsExtendedReport(buffer, bufferLength, blockType, extendedReportLength))
			{
				return nullptr;
			}

			return UnknownExtendedReport::ParseStrict(buffer, bufferLength, extendedReportLength);
		}

		UnknownExtendedReport* UnknownExtendedReport::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength)
		{
			MS_TRACE();

			auto* const extendedReport =
			  new UnknownExtendedReport(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			extendedReport->SetLength(extendedReportLength);

			return extendedReport;
		}

		/* Instance methods. */

		UnknownExtendedReport::UnknownExtendedReport(uint8_t* buffer, size_t bufferLength)
		  : ExtendedReport(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ExtendedReport::ExtendedReportHeaderLength);
		}

		UnknownExtendedReport::~UnknownExtendedReport()
		{
			MS_TRACE();
		}

		void UnknownExtendedReport::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::UnknownExtendedReport>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unknown value length: %zu (has unknown value: %s)",
			  GetUnknownValueLength(),
			  HasUnknownValue() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</RTCP::UnknownExtendedReport>");
		}

		UnknownExtendedReport* UnknownExtendedReport::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedExtendedReport = new UnknownExtendedReport(buffer, bufferLength);

			CloneInto(clonedExtendedReport);

			return clonedExtendedReport;
		}

		UnknownExtendedReport* UnknownExtendedReport::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedExtendedReport =
			  new UnknownExtendedReport(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedExtendedReport);

			return softClonedExtendedReport;
		}
	} // namespace RTCP
} // namespace RTC
