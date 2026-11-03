#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerXrPacket.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/UnknownExtendedReport.hpp"
#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerDlrrExtendedReport.hpp"
#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerRrtExtendedReport.hpp"
#include "fuzzer/include/RTC/NEW_RTCP/packet/extendedReports/FuzzerUnknownExtendedReport.hpp"

void FuzzerRtcRtcpXrPacket::Fuzz(const RTC::NEW_RTCP::XrPacket* packet)
{
	packet->GetSsrc();
	packet->HasExtendedReports();
	packet->GetExtendedReportsCount();
	packet->GetExtendedReportAt(0);
	packet->GetExtendedReportAt(1);
	packet->GetExtendedReportAt(666);
	packet->GetFirstExtendedReportOfType<RTC::NEW_RTCP::RrtExtendedReport>();
	packet->GetFirstExtendedReportOfType<RTC::NEW_RTCP::DlrrExtendedReport>();
	packet->GetFirstExtendedReportOfType<RTC::NEW_RTCP::UnknownExtendedReport>();

	for (auto it = packet->ExtendedReportsBegin(); it != packet->ExtendedReportsEnd(); ++it)
	{
		const RTC::NEW_RTCP::ExtendedReport* const extendedReport = *it;

		extendedReport->GetType();
		extendedReport->HasUnknownType();

		switch (extendedReport->GetType())
		{
			case RTC::NEW_RTCP::ExtendedReport::BlockType::RRT:
			{
				const auto* const rrtExtendedReport =
				  static_cast<const RTC::NEW_RTCP::RrtExtendedReport*>(extendedReport);

				FuzzerRtcRtcpRrtExtendedReport::Fuzz(rrtExtendedReport);

				break;
			}

			case RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR:
			{
				const auto* const dlrrExtendedReport =
				  static_cast<const RTC::NEW_RTCP::DlrrExtendedReport*>(extendedReport);

				FuzzerRtcRtcpDlrrExtendedReport::Fuzz(dlrrExtendedReport);

				break;
			}

			default:
			{
				const auto* const unknownExtendedReport =
				  static_cast<const RTC::NEW_RTCP::UnknownExtendedReport*>(extendedReport);

				FuzzerRtcRtcpUnknownExtendedReport::Fuzz(unknownExtendedReport);
			}
		}
	}
}
