#include "common.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RemoteCaptureTimeEstimator.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("RemoteCaptureTimeEstimator", "[rtp][rtcp][remotecapturetimeestimator]")
{
	// Wall clock of the remote sender when it generates its first Sender Report
	// (seconds since 1900).
	constexpr uint32_t RemoteBaseNtpSec{ 3976000000 };
	// RTP timestamp that first Sender Report reports about.
	constexpr uint32_t RemoteBaseTs{ 1000000 };
	// Our own monotonic clock when that first Sender Report arrives.
	constexpr uint64_t LocalBaseMs{ 1000000 };
	constexpr uint32_t ClockRate{ 90000 };
	constexpr uint32_t Ssrc{ 1111 };

	class RtpStreamRecvListener : public RTC::RTP::RtpStreamRecv::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamSendRtcpPacket(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, RTC::RTCP::Packet* /*packet*/) override
		{
		}

		void OnRtpStreamNeedWorstRemoteFractionLost(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t& /*worstRemoteFractionLost*/) override
		{
		}
	};

	uint64_t nowMs{ LocalBaseMs };

	mocks::MockShared shared(/*getTimeMs*/
	                         [&nowMs]() -> uint64_t
	                         {
		                         return nowMs;
	                         });

	RTC::RTP::RtpStream::Params params;

	params.ssrc      = Ssrc;
	params.clockRate = ClockRate;

	RtpStreamRecvListener listener;
	RTC::RTP::RtpStreamRecv rtpStream(
	  std::addressof(listener),
	  std::addressof(shared),
	  params,
	  /*sendNackDelayMs*/ 0,
	  /*useRtpInactivityCheck*/ false);

	RTC::RemoteCaptureTimeEstimator estimator;

	// Makes the Sender Report about `RemoteBaseTs` plus `idx` seconds of media reach
	// us with no delay at all, and feeds it to the estimator.
	auto receiveSenderReport = [&nowMs, &rtpStream, &estimator](uint32_t idx) -> void
	{
		nowMs = LocalBaseMs + (idx * 1000);

		RTC::RTCP::SenderReport report;

		report.SetSsrc(Ssrc);
		report.SetNtpSec(RemoteBaseNtpSec + idx);
		report.SetNtpFrac(0);
		report.SetRtpTs(RemoteBaseTs + (idx * ClockRate));

		rtpStream.ReceiveRtcpSenderReport(std::addressof(report));
		estimator.SenderReportReceived(std::addressof(rtpStream));
	};

	SECTION("no source until the first Producer has been taken into account")
	{
		REQUIRE_FALSE(estimator.GetSource().has_value());
		REQUIRE_FALSE(estimator.GetLocalCaptureMs(std::addressof(rtpStream), RemoteBaseTs).has_value());
	}

	SECTION("the first Producer chooses abs-capture-time when it negotiated it")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);

		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME);
	}

	SECTION("the first Producer chooses Sender Report when it did not negotiate abs-capture-time")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ false);

		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::SENDER_REPORT);
	}

	SECTION("a Producer without abs-capture-time moves the source to Sender Report")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ false);

		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::SENDER_REPORT);
	}

	SECTION("the source never moves back to abs-capture-time")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ false);
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);

		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::SENDER_REPORT);
	}

	SECTION("the capture instant is translated into our clock")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ false);

		// A single Sender Report is not enough for the clock offset to be estimated.
		receiveSenderReport(0);

		REQUIRE_FALSE(estimator.GetLocalCaptureMs(std::addressof(rtpStream), RemoteBaseTs).has_value());

		for (uint32_t idx{ 1 }; idx < RTC::RemoteClockOffsetEstimator::MinSampleCount; ++idx)
		{
			receiveSenderReport(idx);
		}

		// The Sender Reports reached us with no delay, so the capture instant of the
		// RTP timestamp of the last one is the very instant at which it arrived.
		const auto lastIdx = RTC::RemoteClockOffsetEstimator::MinSampleCount - 1;
		const auto lastTs  = static_cast<uint32_t>(RemoteBaseTs + (lastIdx * ClockRate));

		REQUIRE(estimator.GetLocalCaptureMs(std::addressof(rtpStream), lastTs).has_value());
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  estimator.GetLocalCaptureMs(std::addressof(rtpStream), lastTs).value() ==
		  LocalBaseMs + (lastIdx * 1000));

		// One second of media later maps one second later in our clock too.
		const auto nextTs = static_cast<uint32_t>(RemoteBaseTs + ((lastIdx + 1) * ClockRate));

		REQUIRE(estimator.GetLocalCaptureMs(std::addressof(rtpStream), nextTs).has_value());
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  estimator.GetLocalCaptureMs(std::addressof(rtpStream), nextTs).value() ==
		  LocalBaseMs + ((lastIdx + 1) * 1000));
	}

	SECTION("no capture instant while the abs-capture-time source has nothing to offer")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);

		for (uint32_t idx{ 0 }; idx < RTC::RemoteClockOffsetEstimator::MinSampleCount; ++idx)
		{
			receiveSenderReport(idx);
		}

		// Sender Reports have been received, but the source in use is not allowed to
		// fall back to them.
		REQUIRE_FALSE(estimator.GetLocalCaptureMs(std::addressof(rtpStream), RemoteBaseTs).has_value());
	}
}
