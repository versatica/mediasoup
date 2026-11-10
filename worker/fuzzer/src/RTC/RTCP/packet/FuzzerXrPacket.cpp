#include "fuzzer/include/RTC/RTCP/packet/FuzzerXrPacket.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/UnknownExtendedReport.hpp"
#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerDlrrExtendedReport.hpp"
#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerRrtExtendedReport.hpp"
#include "fuzzer/include/RTC/RTCP/packet/extendedReports/FuzzerUnknownExtendedReport.hpp"

void FuzzerRtcRtcpXrPacket::Fuzz(const RTC::RTCP::XrPacket* packet)
{
	packet->GetSsrc();
	packet->HasExtendedReports();
	packet->GetExtendedReportsCount();
	packet->GetExtendedReportAt(0);
	packet->GetExtendedReportAt(1);
	packet->GetExtendedReportAt(666);
	packet->GetFirstExtendedReportOfType<RTC::RTCP::RrtExtendedReport>();
	packet->GetFirstExtendedReportOfType<RTC::RTCP::DlrrExtendedReport>();
	packet->GetFirstExtendedReportOfType<RTC::RTCP::UnknownExtendedReport>();

	for (auto it = packet->ExtendedReportsBegin(); it != packet->ExtendedReportsEnd(); ++it)
	{
		const RTC::RTCP::ExtendedReport* const extendedReport = *it;

		extendedReport->GetType();
		extendedReport->HasUnknownType();

		switch (extendedReport->GetType())
		{
			case RTC::RTCP::ExtendedReport::BlockType::RRT:
			{
				const auto* const rrtExtendedReport =
				  static_cast<const RTC::RTCP::RrtExtendedReport*>(extendedReport);

				FuzzerRtcRtcpRrtExtendedReport::Fuzz(rrtExtendedReport);

				break;
			}

			case RTC::RTCP::ExtendedReport::BlockType::DLRR:
			{
				const auto* const dlrrExtendedReport =
				  static_cast<const RTC::RTCP::DlrrExtendedReport*>(extendedReport);

				FuzzerRtcRtcpDlrrExtendedReport::Fuzz(dlrrExtendedReport);

				break;
			}

			default:
			{
				const auto* const unknownExtendedReport =
				  static_cast<const RTC::RTCP::UnknownExtendedReport*>(extendedReport);

				FuzzerRtcRtcpUnknownExtendedReport::Fuzz(unknownExtendedReport);
			}
		}
	}
}
