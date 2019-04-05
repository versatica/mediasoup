#define MS_CLASS "RTC::RtpStreamRecv"
// #define MS_LOG_DEV

#include "RTC/RtpStreamRecv.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/Codecs/Codecs.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t InactivityCheckPeriod{ 250 };

	/* Instance methods. */

	RtpStreamRecv::RtpStreamRecv(RTC::RtpStreamRecv::Listener* listener, RTC::RtpStream::Params& params)
	  : RTC::RtpStream::RtpStream(listener, params, 10)
	{
		MS_TRACE();

		if (this->params.useNack)
			this->nackGenerator.reset(new RTC::NackGenerator(this));

		// Set the RTP inactivity check periodic timer.
		this->inactivityCheckPeriodicTimer = new Timer(this);

		// Run the RTP inactivity periodic timer (unless DTX is enabled).
		if (!this->params.useDtx)
		{
			this->inactive = false;
			this->inactivityCheckPeriodicTimer->Start(InactivityCheckPeriod, InactivityCheckPeriod);
		}
	}

	RtpStreamRecv::~RtpStreamRecv()
	{
		MS_TRACE();

		// Close the RTP inactivity check periodic timer.
		delete this->inactivityCheckPeriodicTimer;
	}

	void RtpStreamRecv::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		RTC::RtpStream::FillJsonStats(jsonObject);

		jsonObject["type"]   = "inbound-rtp";
		jsonObject["jitter"] = this->jitter;
	}

	bool RtpStreamRecv::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RTC::RtpStream::ReceivePacket(packet))
		{
			MS_WARN_TAG(rtp, "packet discarded");

			return false;
		}

		// Calculate Jitter.
		CalculateJitter(packet->GetTimestamp());

		// Process the packet at codec level.
		if (packet->GetPayloadType() == GetPayloadType())
			RTC::Codecs::ProcessRtpPacket(packet, GetMimeType());

		// Pass the packet to the NackGenerator if there is no RTX, so if the
		// NackGenerator returns true it means that it was a NACKed packet.
		if (this->params.useNack && !HasRtx() && this->nackGenerator->ReceivePacket(packet))
		{
			// Mark the packet as retransmitted and repaired.
			RTC::RtpStream::PacketRetransmitted(packet);
			RTC::RtpStream::PacketRepaired(packet);
		}

		// Ensure the inactivityCheckPeriodicTimer runs (unless DTX is enabled).
		if (!this->params.useDtx && !this->inactivityCheckPeriodicTimer->IsActive())
		{
			this->inactive = false;
			this->inactivityCheckPeriodicTimer->Restart();
		}

		// Increase transmission counter.
		this->transmissionCounter.Update(packet);

		return true;
	}

	bool RtpStreamRecv::ReceiveRtxPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->params.useNack)
		{
			MS_WARN_TAG(rtx, "NACK not supported");

			return false;
		}

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
		if (!RTC::RtpStream::UpdateSeq(packet))
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
			RTC::Codecs::ProcessRtpPacket(packet, GetMimeType());

		// Mark the packet as retransmitted.
		RTC::RtpStream::PacketRetransmitted(packet);

		// Pass the packet to the NackGenerator and return true just if this was a
		// NACKed packet.
		if (this->nackGenerator->ReceivePacket(packet))
		{
			// Mark the packet as repaired.
			RTC::RtpStream::PacketRepaired(packet);

			// Increase transmission counter.
			this->transmissionCounter.Update(packet);

			return true;
		}

		return false;
	}

	RTC::RTCP::ReceiverReport* RtpStreamRecv::GetRtcpReceiverReport()
	{
		MS_TRACE();

		uint8_t worstRemoteFractionLost{ 0 };

		if (this->params.useInBandFec)
		{
			// Notify the listener so we'll get the worst remote fraction lost.
			static_cast<RTC::RtpStreamRecv::Listener*>(this->listener)
			  ->OnRtpStreamNeedWorstRemoteFractionLost(this, worstRemoteFractionLost);

			if (worstRemoteFractionLost > 0)
				MS_DEBUG_TAG(rtcp, "using worst remote fraction lost:%" PRIu8, worstRemoteFractionLost);
		}

		auto report = new RTC::RTCP::ReceiverReport();

		report->SetSsrc(GetSsrc());

		uint32_t prevPacketsLost = this->packetsLost;

		// Calculate Packets Expected and Lost.
		auto expected = this->GetExpectedPackets();

		this->packetsLost = expected - this->transmissionCounter.GetPacketCount();

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

		// Worst remote fraction lost is not worse than local one.
		if (worstRemoteFractionLost <= this->fractionLost)
		{
			this->reportedPacketLost += (this->packetsLost - prevPacketsLost);

			report->SetTotalLost(this->reportedPacketLost);
			report->SetFractionLost(this->fractionLost);
		}
		else
		{
			// Recalculate packetsLost.
			uint32_t newLostInterval     = (worstRemoteFractionLost * expectedInterval) >> 8;
			uint32_t newReceivedInterval = expectedInterval - newLostInterval;

			this->reportedPacketLost += (receivedInterval - newReceivedInterval);

			report->SetTotalLost(this->reportedPacketLost);
			report->SetFractionLost(worstRemoteFractionLost);
		}

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

		// Update the score with the current RR.
		UpdateScore();
	}

	void RtpStreamRecv::RequestKeyFrame()
	{
		MS_TRACE();

		if (this->params.usePli)
		{
			// Reset NackGenerator.
			if (this->params.useNack)
				this->nackGenerator->Reset();

			MS_DEBUG_2TAGS(rtcp, rtx, "sending PLI [ssrc:%" PRIu32 "]", GetSsrc());

			RTC::RTCP::FeedbackPsPliPacket packet(0, GetSsrc());

			packet.Serialize(RTC::RTCP::Buffer);

			this->pliCount++;

			// Notify the listener.
			static_cast<RTC::RtpStreamRecv::Listener*>(this->listener)->OnRtpStreamSendRtcpPacket(this, &packet);
		}
		else if (this->params.useFir)
		{
			// Reset NackGenerator.
			if (this->params.useNack)
				this->nackGenerator->Reset();

			MS_DEBUG_2TAGS(rtcp, rtx, "sending FIR [ssrc:%" PRIu32 "]", GetSsrc());

			RTC::RTCP::FeedbackPsFirPacket packet(0, GetSsrc());
			auto* item = new RTC::RTCP::FeedbackPsFirItem(GetSsrc(), ++this->firSeqNumber);

			packet.AddItem(item);
			packet.Serialize(RTC::RTCP::Buffer);

			this->firCount++;

			// Notify the listener.
			static_cast<RTC::RtpStreamRecv::Listener*>(this->listener)->OnRtpStreamSendRtcpPacket(this, &packet);
		}
	}

	void RtpStreamRecv::Pause()
	{
		MS_TRACE();

		this->inactive = true;
		this->inactivityCheckPeriodicTimer->Stop();
	}

	void RtpStreamRecv::Resume()
	{
		MS_TRACE();

		// Restart the RTP inactivity periodic timer (unless DTX is enabled).
		if (!this->params.useDtx)
		{
			this->inactive = false;
			this->inactivityCheckPeriodicTimer->Restart();
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

	void RtpStreamRecv::UpdateScore()
	{
		MS_TRACE();

		// Calculate number of packets expected in this interval.
		auto totalExpected = GetExpectedPackets();
		uint32_t expected  = totalExpected - this->expectedPrior;

		this->expectedPrior = totalExpected;

		// Calculate number of packets received in this interval.
		auto totalReceived = this->transmissionCounter.GetPacketCount();
		uint32_t received  = totalReceived - this->receivedPrior;

		this->receivedPrior = totalReceived;

		// Calculate number of packets lost in this interval.
		uint32_t lost;

		if (expected < received)
			lost = 0;
		else
			lost = expected - received;

		// Calculate number of packets repaired in this interval.
		auto totalRepaired = this->packetsRepaired;
		uint32_t repaired  = totalRepaired - this->repairedPrior;

		this->repairedPrior = totalRepaired;

		// Calculate number of packets retransmitted in this interval.
		auto totatRetransmitted = this->packetsRetransmitted;
		uint32_t retransmitted  = totatRetransmitted - this->retransmittedPrior;

		this->retransmittedPrior = totatRetransmitted;

		if (this->inactive)
			return;

		// We didn't expect more packets to come.
		if (expected == 0)
		{
			RTC::RtpStream::UpdateScore(10);

			return;
		}

		if (repaired > lost)
			repaired = lost;

		MS_ERROR(
		  "fixed values [expected:%" PRIu32 ",received:%" PRIu32 ", lost:%" PRIu32 ", repaired:%" PRIu32
		  ", retransmitted:%" PRIu32,
		  expected,
		  received,
		  lost,
		  repaired,
		  retransmitted);

		float repairedRatio = repaired / received;
		auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

		MS_ASSERT(retransmitted >= repaired, "repaired packets cannot be more than retransmitted ones");

		if (retransmitted > 0)
			repairedWeight *= repaired / retransmitted;

		lost -= repaired * repairedWeight;

		MS_ERROR(
		  "[repairedRatio:%f, repairedWeight:%f, new lost:%" PRIu32 "]",
		  repairedRatio,
		  repairedWeight,
		  lost);

		float deliveredRatio = static_cast<float>(received - lost) / static_cast<float>(received);

		auto score = std::round(std::pow(deliveredRatio, 4) * 10);

		MS_ERROR("RESULT [deliveredRatio:%f, score:%f]", deliveredRatio, score);

		MS_ERROR("==========\n");

#ifdef MS_LOG_DEV
		MS_DEBUG_TAG(
		  score,
		  "[totalExpected:%" PRIu32 ", totalReceived:%zu, totalRepaired:%zu, expected:%" PRIu32
		  ", received:%" PRIu32 ", repaired:%" PRIu32 ", lossPercentage:%f, score:%" PRIu8 "]",
		  totalExpected,
		  totalReceived,
		  totalRepaired,
		  expected,
		  received,
		  repaired,
		  lossPercentage,
		  score);
#endif

		RtpStream::UpdateScore(score);
	}

	inline void RtpStreamRecv::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->inactivityCheckPeriodicTimer)
		{
			auto now = DepLibUV::GetTime();

			// If no RTP is being received reset the score.
			if (this->transmissionCounter.GetRate(now) == 0)
			{
				this->inactive = true;
				this->inactivityCheckPeriodicTimer->Stop();

				if (GetScore() != 0)
					MS_WARN_2TAGS(rtp, score, "RTP inactivity detected, resetting score to 0");

				ResetScore(0, true);
			}
		}
	}

	inline void RtpStreamRecv::OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers)
	{
		MS_TRACE();

		MS_ASSERT(this->params.useNack, "NACK required but not supported");

		MS_DEBUG_TAG(
		  rtx,
		  "triggering NACK [ssrc:%" PRIu32 ", first seq:%" PRIu16 ", num packets:%zu]",
		  this->params.ssrc,
		  seqNumbers[0],
		  seqNumbers.size());

		RTC::RTCP::FeedbackRtpNackPacket packet(0, GetSsrc());

		auto it        = seqNumbers.begin();
		const auto end = seqNumbers.end();
		size_t numPacketsRequested{ 0 };

		while (it != end)
		{
			uint16_t seq;
			uint16_t bitmask{ 0 };

			seq = *it;
			++it;

			while (it != end)
			{
				uint16_t shift = *it - seq - 1;

				if (shift > 15)
					break;

				bitmask |= (1 << shift);
				++it;
			}

			auto* nackItem = new RTC::RTCP::FeedbackRtpNackItem(seq, bitmask);

			packet.AddItem(nackItem);

			numPacketsRequested += nackItem->CountRequestedPackets();
		}

		// Ensure that the RTCP packet fits into the RTCP buffer.
		if (packet.GetSize() > RTC::RTCP::BufferSize)
		{
			MS_WARN_TAG(rtx, "cannot send RTCP NACK packet, size too big (%zu bytes)", packet.GetSize());

			return;
		}

		this->nackCount++;
		this->nackPacketCount += numPacketsRequested;

		packet.Serialize(RTC::RTCP::Buffer);

		// Notify the listener.
		static_cast<RTC::RtpStreamRecv::Listener*>(this->listener)->OnRtpStreamSendRtcpPacket(this, &packet);
	}

	inline void RtpStreamRecv::OnNackGeneratorKeyFrameRequired()
	{
		MS_TRACE();

		MS_DEBUG_TAG(rtx, "requesting key frame [ssrc:%" PRIu32 "]", this->params.ssrc);

		RequestKeyFrame();
	}
} // namespace RTC
