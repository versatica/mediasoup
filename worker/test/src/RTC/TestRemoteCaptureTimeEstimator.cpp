#include "common.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RemoteCaptureTimeEstimator.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
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
	constexpr int64_t LocalBaseUs{ 1000000 * 1000 };
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

		uint8_t OnRtpStreamNeedWorstRemoteFractionLost(RTC::RTP::RtpStreamRecv* /*rtpStream*/) override
		{
			return 0;
		}

		void OnRtpStreamSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/) override
		{
		}
	};

	int64_t nowUs{ LocalBaseUs };

	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
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
	auto receiveSenderReport = [&nowUs, &shared, &rtpStream, &estimator](uint32_t idx) -> void
	{
		nowUs = LocalBaseUs + (idx * 1000000);

		RTC::RTCP::SenderReport report;

		report.SetSsrc(Ssrc);
		report.SetNtpSec(RemoteBaseNtpSec + idx);
		report.SetNtpFrac(0);
		report.SetRtpTs(RemoteBaseTs + (idx * ClockRate));

		rtpStream.ReceiveRtcpSenderReport(std::addressof(report), shared.GetTimeUs());
		estimator.SenderReportReceived(std::addressof(rtpStream));
	};

	// Makes a packet carrying the 'abs-capture-time' extension reach the stream, which
	// is what tells a sender that does send it from one that only announced it.
	auto receiveAbsCaptureTimePacket = [&nowUs, &rtpStream]() -> void
	{
		// Id the extension is given, which only has to match between what is written
		// and what is read back.
		constexpr uint8_t AbsCaptureTimeId{ 1 };

		std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		REQUIRE(packet);

		packet->SetSsrc(Ssrc);
		packet->SetTimestamp(RemoteBaseTs);

		const std::vector<RTC::RTP::Packet::Extension> extensions{
			{ RTC::RtpHeaderExtensionUri::Type::ABS_CAPTURE_TIME,
			 AbsCaptureTimeId, /*len*/ 8,
			 rtpCommon::DataBuffer }
		};

		packet->SetExtensions(RTC::RTP::Packet::ExtensionsType::OneByte, extensions);

		RTC::RTP::HeaderExtensionIds headerExtensionIds;

		headerExtensionIds.absCaptureTime = AbsCaptureTimeId;

		packet->AssignExtensionIds(headerExtensionIds);

		const auto ntp = Utils::Time::TimeUsToNtp(static_cast<int64_t>(RemoteBaseNtpSec) * 1000000);

		REQUIRE(packet->UpdateAbsCaptureTime(
		  (static_cast<uint64_t>(ntp.seconds) << 32) | static_cast<uint64_t>(ntp.fractions)));

		rtpStream.ReceivePacket(packet.get(), nowUs);
	};

	SECTION("no source until the first Producer has been taken into account")
	{
		REQUIRE_FALSE(estimator.GetSource().has_value());
		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());
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

	SECTION("abs-capture-time negotiated but never received moves the source to Sender Report")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);

		// The first packet is what the wait is measured against, so nothing happens on
		// it however long this sender has been around.
		nowUs += 60 * 1000000;

		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());
		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME);

		// Still within the wait, since the extension travels with the first packet of
		// every frame and may simply not have arrived yet.
		nowUs += 4 * 1000000;

		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());
		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME);

		// Long enough without a single packet carrying it.
		nowUs += 1 * 1000000;

		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());
		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::SENDER_REPORT);
	}

	SECTION("abs-capture-time received keeps the source whatever the sender does later")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ true);

		receiveAbsCaptureTimePacket();

		REQUIRE(rtpStream.HasAbsCaptureTime());

		// Way past the wait, which is not armed at all once the extension has been seen.
		nowUs += 60 * 1000000;

		estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs);

		REQUIRE(estimator.GetSource() == RTC::RemoteCaptureTimeEstimator::Source::ABS_CAPTURE_TIME);
	}

	SECTION("the capture instant is translated into our clock")
	{
		estimator.UpdateSource(/*absCaptureTimeNegotiated*/ false);

		// A single Sender Report is not enough for the clock offset to be estimated.
		receiveSenderReport(0);

		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());

		for (uint32_t idx{ 1 }; idx < RTC::RemoteClockOffsetEstimator::MinSampleCount; ++idx)
		{
			receiveSenderReport(idx);
		}

		// The Sender Reports reached us with no delay, so the capture instant of the
		// RTP timestamp of the last one is the very instant at which it arrived.
		const auto lastIdx = RTC::RemoteClockOffsetEstimator::MinSampleCount - 1;
		const auto lastTs  = static_cast<uint32_t>(RemoteBaseTs + (lastIdx * ClockRate));

		REQUIRE(estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), lastTs, nowUs).has_value());
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), lastTs, nowUs).value() ==
		  LocalBaseUs + (lastIdx * 1000000));

		// One second of media later maps one second later in our clock too.
		const auto nextTs = static_cast<uint32_t>(RemoteBaseTs + ((lastIdx + 1) * ClockRate));

		REQUIRE(estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), nextTs, nowUs).has_value());
		REQUIRE(
		  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), nextTs, nowUs).value() ==
		  LocalBaseUs + ((lastIdx + 1) * 1000000));
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
		REQUIRE_FALSE(
		  estimator.GetLocalCaptureAtUs(std::addressof(rtpStream), RemoteBaseTs, nowUs).has_value());
	}
}
