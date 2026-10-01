#define MS_CLASS "RTC::RemoteCaptureTimeEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RemoteCaptureTimeEstimator.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	// How long a sender that negotiated 'abs-capture-time' may go without sending it
	// before the capture instant is read from its Sender Reports instead. It only has
	// to outlast the first packets of a sender that does send it, since the extension
	// travels with the first packet of every frame.
	static constexpr int64_t AbsCaptureTimeTimeoutUs{ 5 * 1000 * 1000 };

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

	void RemoteCaptureTimeEstimator::MayFallBackToSenderReport(
	  const RTC::RTP::RtpStreamRecv* rtpStream, int64_t nowUs)
	{
		MS_TRACE();

		if (this->source != RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME)
		{
			return;
		}

		// Some stream of this sender has carried the extension at some point, so it is
		// being sent and whatever stream has none of it is just early.
		if (this->absCaptureTimeReceived)
		{
			return;
		}

		if (rtpStream->HasAbsCaptureTime())
		{
			this->absCaptureTimeReceived = true;

			return;
		}

		if (!this->firstPacketAtUs.has_value())
		{
			this->firstPacketAtUs = nowUs;

			return;
		}

		if (nowUs - this->firstPacketAtUs.value() < AbsCaptureTimeTimeoutUs)
		{
			return;
		}

		MS_WARN_2TAGS(
		  rtp,
		  rtcp,
		  "abs-capture-time negotiated but never received, reading the capture instant from Sender Reports instead");

		this->source = RemoteCaptureTimeEstimator::Source::SENDER_REPORT;
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
		  static_cast<int64_t>(rtpStream->GetRttMs()));
	}

	std::optional<int64_t> RemoteCaptureTimeEstimator::GetLocalCaptureAtUs(
	  const RTC::RTP::RtpStreamRecv* rtpStream, uint32_t ts, int64_t nowUs)
	{
		MS_TRACE();

		if (!this->source.has_value())
		{
			return std::nullopt;
		}

		MayFallBackToSenderReport(rtpStream, nowUs);

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
