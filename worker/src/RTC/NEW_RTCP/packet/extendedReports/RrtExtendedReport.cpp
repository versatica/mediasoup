#define MS_CLASS "RTC::NEW_RTCP::RrtExtendedReport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		RrtExtendedReport* RrtExtendedReport::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			ExtendedReport::BlockType blockType;
			size_t extendedReportLength;

			if (!ExtendedReport::IsExtendedReport(buffer, bufferLength, blockType, extendedReportLength))
			{
				return nullptr;
			}

			if (blockType != ExtendedReport::BlockType::RRT)
			{
				MS_WARN_DEV("invalid block type");

				return nullptr;
			}

			return RrtExtendedReport::ParseStrict(buffer, bufferLength, extendedReportLength);
		}

		RrtExtendedReport* RrtExtendedReport::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < RrtExtendedReport::RrtExtendedReportLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const extendedReport = new RrtExtendedReport(buffer, bufferLength);

			extendedReport->InitializeHeader(
			  ExtendedReport::BlockType::RRT, 0, RrtExtendedReport::RrtExtendedReportLength);

			// Must also initialize extra fields in the header.
			extendedReport->SetNtp({ .seconds = 0, .fractions = 0 });

			// No need to invoke SetLength() since constructor invoked it with
			// RrtExtendedReport length.

			return extendedReport;
		}

		RrtExtendedReport* RrtExtendedReport::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength)
		{
			MS_TRACE();

			if (extendedReportLength != RrtExtendedReport::RrtExtendedReportLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "RrtExtendedReport length must be %zu (got %zu)",
				  RrtExtendedReport::RrtExtendedReportLength,
				  extendedReportLength);

				return nullptr;
			}

			auto* const extendedReport = new RrtExtendedReport(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			extendedReport->SetLength(extendedReportLength);

			return extendedReport;
		}

		/* Instance methods. */

		RrtExtendedReport::RrtExtendedReport(uint8_t* buffer, size_t bufferLength)
		  : ExtendedReport(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(RrtExtendedReport::RrtExtendedReportLength);
		}

		RrtExtendedReport::~RrtExtendedReport()
		{
			MS_TRACE();
		}

		void RrtExtendedReport::Dump(int indentation) const
		{
			MS_TRACE();

			const Utils::Time::Ntp ntp = GetNtp();

			MS_DUMP_CLEAN(indentation, "<RTCP::RrtExtendedReport>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ntp sec: %" PRIu32, ntp.seconds);
			MS_DUMP_CLEAN(indentation, "  ntp frac: %" PRIu32, ntp.fractions);
			MS_DUMP_CLEAN(indentation, "</RTCP::RrtExtendedReport>");
		}

		RrtExtendedReport* RrtExtendedReport::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedExtendedReport = new RrtExtendedReport(buffer, bufferLength);

			CloneInto(clonedExtendedReport);

			return clonedExtendedReport;
		}

		void RrtExtendedReport::SetNtp(const Utils::Time::Ntp& ntp)
		{
			MS_TRACE();

			auto* const buffer = const_cast<uint8_t*>(GetBuffer());

			Utils::Byte::Set4Bytes(buffer, 4, ntp.seconds);
			Utils::Byte::Set4Bytes(buffer, 8, ntp.fractions);
		}

		RrtExtendedReport* RrtExtendedReport::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedExtendedReport =
			  new RrtExtendedReport(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedExtendedReport);

			return softClonedExtendedReport;
		}
	} // namespace NEW_RTCP
} // namespace RTC
