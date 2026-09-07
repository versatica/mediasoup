#define MS_CLASS "RTC::RemoteCaptureTimeEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RemoteCaptureTimeEstimator.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	void RemoteCaptureTimeEstimator::UpdateSource(bool absCaptureTimeNegotiated)
	{
		MS_TRACE();

		// Once the Sender Report source has been chosen there is no way back.
		if (this->source == RemoteCaptureTimeEstimator::Source::SENDER_REPORT)
		{
			if (absCaptureTimeNegotiated)
			{
				MS_WARN_2TAGS(rtp, rtcp, "cannot move capture instant source back to abs-capture-time");
			}

			return;
		}

		this->source = absCaptureTimeNegotiated ? RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME
		                                        : RemoteCaptureTimeEstimator::Source::SENDER_REPORT;

		MS_DEBUG_2TAGS(
		  rtp,
		  rtcp,
		  "capture instant source set to %s",
		  this->source == RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME ? "abs-capture-time"
			                                                                     : "Sender Report");
	}

	void RemoteCaptureTimeEstimator::SenderReportReceived(const RTC::RTP::RtpStreamRecv* rtpStream)
	{
		MS_TRACE();

		const auto senderReportMapping      = rtpStream->GetSenderReportMapping();
		const auto senderReportReceivedAtUs = rtpStream->GetSenderReportReceivedAtUs();

		// Both are stored together, so either both are set or none of them is.
		if (!senderReportMapping.has_value() || !senderReportReceivedAtUs.has_value())
		{
			return;
		}

		this->clockOffsetEstimator.AddSenderReport(
		  senderReportMapping.value().ntpUs,
		  senderReportReceivedAtUs.value(),
		  static_cast<uint32_t>(rtpStream->GetRtt()));
	}

	std::optional<int64_t> RemoteCaptureTimeEstimator::GetLocalCaptureAtUs(
	  const RTC::RTP::RtpStreamRecv* rtpStream, uint32_t ts) const
	{
		MS_TRACE();

		if (!this->source.has_value())
		{
			return std::nullopt;
		}

		const auto remoteCaptureAtUs = this->source == RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME
		                                 ? rtpStream->GetRemoteCaptureAtUsFromAbsCaptureTime(ts)
		                                 : rtpStream->GetRemoteCaptureAtUsFromSenderReport(ts);

		if (!remoteCaptureAtUs.has_value())
		{
			return std::nullopt;
		}

		return this->clockOffsetEstimator.RemoteUsToLocalUs(remoteCaptureAtUs.value());
	}
} // namespace RTC
