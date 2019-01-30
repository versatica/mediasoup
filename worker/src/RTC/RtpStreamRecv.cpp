#define MS_CLASS "RTC::RtpStreamRecv"
// #define MS_LOG_DEV

#include "RTC/RtpStreamRecv.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t StatusCheckPeriod{ 250 };

	/* Instance methods. */

	RtpStreamRecv::RtpStreamRecv(Listener* listener, RTC::RtpStream::Params& params)
	  : RtpStream::RtpStream(params), listener(listener)
	{
		MS_TRACE();

		if (this->params.useNack)
			this->nackGenerator.reset(new RTC::NackGenerator(this));

		this->rtpMonitor.reset(new RtpStreamMonitor(this, this));

		// Start the RTP monitor with a possitive score.
		this->rtpMonitor->AddScore(10);

		// Set the incactivity check periodic timer.
		this->inactivityCheckPeriodicTimer = new Timer(this);

		// Run the timers.
		this->inactivityCheckPeriodicTimer->Start(StatusCheckPeriod, StatusCheckPeriod);
		this->rtcpReportCheckTimer->Start(2000);
	}

	RtpStreamRecv::~RtpStreamRecv()
	{
		MS_TRACE();

		// Close the incactivity check periodic timer.
		delete this->inactivityCheckPeriodicTimer;
	}

	void RtpStreamRecv::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		RtpStream::FillJsonStats(jsonObject);

		jsonObject["type"]   = "inbound-rtp";
		jsonObject["jitter"] = this->jitter;
	}

	bool RtpStreamRecv::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceivePacket(packet))
		{
			MS_WARN_TAG(rtp, "packet discarded");

			return false;
		}

		// Calculate Jitter.
		CalculateJitter(packet->GetTimestamp());

		// Process the packet at codec level.
		if (packet->GetPayloadType() == GetPayloadType())
			Codecs::ProcessRtpPacket(packet, GetMimeType());

		// Pass the packet to the NackGenerator.
		if (this->params.useNack)
			this->nackGenerator->ReceivePacket(packet);

		return true;
	}

	bool RtpStreamRecv::ReceiveRtxPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(packet->GetSsrc() == this->params.rtxSsrc, "invalid ssrc on RTX packet");

		// Check that the payload type corresponds to the one negotiated.
		if (packet->GetPayloadType() != this->params.rtxPayloadType)
		{
			MS_WARN_TAG(
			  rtx,
			  "ignoring RTX packet with invalid payload type [ssrc:%" PRIu32 ", seq:%" PRIu16
			  ", pt:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetPayloadType());

			return false;
		}

		// Get the RTX packet sequence number for logging purposes.
		auto rtxSeq = packet->GetSequenceNumber();

		// Get the original RTP packet.
		if (!packet->RtxDecode(this->params.payloadType, this->params.ssrc))
		{
			MS_DEBUG_TAG(
			  rtx,
			  "ignoring empty RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", pt:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber(),
			  packet->GetPayloadType());

			return false;
		}

		MS_DEBUG_TAG(
		  rtx,
		  "received RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "] recovering original [ssrc:%" PRIu32
		  ", seq:%" PRIu16 "]",
		  this->params.rtxSsrc,
		  rtxSeq,
		  packet->GetSsrc(),
		  packet->GetSequenceNumber());

		// If not a valid packet ignore it.
		if (!UpdateSeq(packet))
		{
			MS_WARN_TAG(
			  rtx,
			  "invalid RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			return false;
		}

		// Process the packet at codec level.
		if (packet->GetPayloadType() == GetPayloadType())
			Codecs::ProcessRtpPacket(packet, GetMimeType());

		// Pass the packet to the NackGenerator and return true just if this was a
		// NACKed packet.
		if (this->params.useNack)
			return this->nackGenerator->ReceivePacket(packet);

		return false;
	}

	RTC::RTCP::ReceiverReport* RtpStreamRecv::GetRtcpReceiverReport()
	{
		MS_TRACE();

		auto report = new RTC::RTCP::ReceiverReport();

		// Calculate Packets Expected and Lost.
		uint32_t expected = (this->cycles + this->maxSeq) - this->baseSeq + 1;
		this->packetsLost = expected - this->transmissionCounter.GetPacketCount();

		report->SetTotalLost(this->packetsLost);

		// Calculate Fraction Lost.
		uint32_t expectedInterval = expected - this->expectedPrior;

		this->expectedPrior = expected;

		uint32_t receivedInterval = this->transmissionCounter.GetPacketCount() - this->receivedPrior;

		this->receivedPrior = transmissionCounter.GetPacketCount();

		int32_t lostInterval = expectedInterval - receivedInterval;

		if (expectedInterval == 0 || lostInterval <= 0)
			this->fractionLost = 0;
		else
			this->fractionLost = (lostInterval << 8) / expectedInterval;

		report->SetFractionLost(this->fractionLost);

		// Fill the rest of the report.
		report->SetLastSeq(static_cast<uint32_t>(this->maxSeq) + this->cycles);
		report->SetJitter(this->jitter);

		if (this->lastSrReceived != 0)
		{
			// Get delay in milliseconds.
			auto delayMs = static_cast<uint32_t>(DepLibUV::GetTime() - this->lastSrReceived);
			// Express delay in units of 1/65536 seconds.
			uint32_t dlsr = (delayMs / 1000) << 16;

			dlsr |= uint32_t{ (delayMs % 1000) * 65536 / 1000 };
			report->SetDelaySinceLastSenderReport(dlsr);
			report->SetLastSenderReport(this->lastSrTimestamp);
		}
		else
		{
			report->SetDelaySinceLastSenderReport(0);
			report->SetLastSenderReport(0);
		}

		return report;
	}

	void RtpStreamRecv::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		MS_TRACE();

		this->lastSrReceived  = DepLibUV::GetTime();
		this->lastSrTimestamp = report->GetNtpSec() << 16;
		this->lastSrTimestamp += report->GetNtpFrac() >> 16;

		// Provide the RTP monitor with the current RR.
		this->rtpMonitor->ReceiveRtcpReceiverReport(this->GetRtcpReceiverReport());

		this->rtcpReportCheckTimer->Stop();
		this->rtcpReportCheckTimer->Start(2000);
	}

	void RtpStreamRecv::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->params.usePli)
		{
			// Reset NackGenerator.
			if (this->params.useNack)
				this->nackGenerator->Reset();

			this->listener->OnRtpStreamRecvPliRequired(this);
		}
		else if (this->params.useFir)
		{
			// Reset NackGenerator.
			if (this->params.useNack)
				this->nackGenerator->Reset();

			this->listener->OnRtpStreamRecvFirRequired(this);
		}
	}

	void RtpStreamRecv::CalculateJitter(uint32_t rtpTimestamp)
	{
		MS_TRACE();

		if (this->params.clockRate == 0u)
			return;

		auto transit =
		  static_cast<int>(DepLibUV::GetTime() - (rtpTimestamp * 1000 / this->params.clockRate));
		int d = transit - this->transit;

		this->transit = transit;
		if (d < 0)
			d = -d;
		this->jitter += (1. / 16.) * (static_cast<double>(d) - this->jitter);
	}

	void RtpStreamRecv::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->inactivityCheckPeriodicTimer)
		{
			auto now = DepLibUV::GetTime();

			// No RTP is being received, reset rtpMonitor and notify the listener.
			if (this->transmissionCounter.GetRate(now) == 0 && this->lastScore != 0)
			{
				this->lastScore = 0;
				this->rtpMonitor->Reset();
				this->listener->OnRtpStreamRecvScore(this, 0);
			}
		}
		else if (timer == this->rtcpReportCheckTimer)
		{
			// TODO: 'figure out' a score based on received RTP or set a fixed value.
			this->rtpMonitor->AddScore(8);
			this->rtcpReportCheckTimer->Start(5000);
		}
	}

	void RtpStreamRecv::OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers)
	{
		MS_TRACE();

		MS_ASSERT(this->params.useNack, "NACK required but not supported");

		MS_DEBUG_TAG(
		  rtx,
		  "triggering NACK [ssrc:%" PRIu32 ", first seq:%" PRIu16 ", num packets:%zu]",
		  this->params.ssrc,
		  seqNumbers[0],
		  seqNumbers.size());

		this->listener->OnRtpStreamRecvNackRequired(this, seqNumbers);
	}

	void RtpStreamRecv::OnNackGeneratorKeyFrameRequired()
	{
		MS_TRACE();

		MS_DEBUG_TAG(rtx, "requesting key frame [ssrc:%" PRIu32 "]", this->params.ssrc);

		RequestKeyFrame();
	}
} // namespace RTC
