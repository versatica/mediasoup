#define MS_CLASS "RTC::RTP::RtpStreamRecv"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/RtpStreamRecv.hpp"
#include "Logger.hpp"
#include "RTC/RTCP/packet/AnyReportPacket.hpp"
#include "RTC/RTCP/packet/FirPacket.hpp"
#include "RTC/RTCP/packet/NackPacket.hpp"
#include "RTC/RTCP/packet/PliPacket.hpp"
#include "RTC/RTP/Codecs/Tools.hpp"
#include <cmath> // std::pow(), std::round()

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		static constexpr int64_t InactivityCheckIntervalMs{ 1500 };
		static constexpr int64_t InactivityCheckIntervalWithDtxMs{ 5000 };
		// How often the spatial layers of a stream are checked for traffic.
		// NOTE: A layer that stops doesn't report zero bitrate until the window of
		// its counter has fully elapsed, so this interval just adds to that
		// unavoidable detection delay.
		static constexpr int64_t SpatialLayersActivityCheckIntervalMs{ 1500 };
		// Smallest difference between the transit times of two packets (in RTP
		// timestamp units) that is left out of the jitter, which is 5 seconds at 90
		// kHz.
		static constexpr int64_t MinIgnoredJitterTransitDifference{ 5 * 90000 };
		// Buffer in which the RTCP feedback packets to send are built.
		// NOTE: Every packet built in it is handed to the listener, which sends it
		// right away, before the buffer is used again.
		alignas(4) static thread_local uint8_t RtcpPacketBuffer[RTC::Consts::OutgoingRtcpPacketMaxSize];

		/* TransmissionCounter methods. */

		RtpStreamRecv::TransmissionCounter::TransmissionCounter(
		  SharedInterface* shared, uint8_t spatialLayers, uint8_t temporalLayers, int64_t windowSizeMs)
		{
			MS_TRACE();

			// Reserve vectors capacity.
			this->spatialLayerCounters = std::vector<std::vector<RTC::RtpDataCounter>>(spatialLayers);

			for (auto& spatialLayerCounter : this->spatialLayerCounters)
			{
				for (uint8_t tIdx{ 0 }; tIdx < temporalLayers; ++tIdx)
				{
					spatialLayerCounter.emplace_back(shared, /*ignorePaddingOnlyPackets*/ true, windowSizeMs);
				}
			}
		}

		void RtpStreamRecv::TransmissionCounter::Update(const RTP::Packet* packet)
		{
			MS_TRACE();

			auto spatialLayer  = packet->GetSpatialLayer();
			auto temporalLayer = packet->GetTemporalLayer();

			// Sanity check. Do not allow spatial layers higher than defined.
			spatialLayer =
			  std::min(static_cast<size_t>(spatialLayer), this->spatialLayerCounters.size() - 1);

			// Sanity check. Do not allow temporal layers higher than defined.
			temporalLayer =
			  std::min(static_cast<size_t>(temporalLayer), this->spatialLayerCounters[0].size() - 1);

			auto& temporalLayerCounter = this->spatialLayerCounters[spatialLayer][temporalLayer];

			temporalLayerCounter.Update(packet);
		}

		int64_t RtpStreamRecv::TransmissionCounter::GetBitrate(int64_t nowMs)
		{
			MS_TRACE();

			int64_t rate{ 0 };

			for (auto& spatialLayerCounter : this->spatialLayerCounters)
			{
				for (auto& temporalLayerCounter : spatialLayerCounter)
				{
					rate += temporalLayerCounter.GetBitrate(nowMs).value_or(0);
				}
			}

			return rate;
		}

		int64_t RtpStreamRecv::TransmissionCounter::GetCumulativeBitrate(
		  int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer)
		{
			MS_TRACE();

			MS_ASSERT(spatialLayer < this->spatialLayerCounters.size(), "spatialLayer too high");
			MS_ASSERT(
			  temporalLayer < this->spatialLayerCounters[spatialLayer].size(), "temporalLayer too high");

			// Return 0 if specified layers are not being received.
			if (GetTemporalLayerBitrate(nowMs, spatialLayer, temporalLayer) <= 0)
			{
				return 0;
			}

			int64_t rate{ 0 };

			// Iterate the given spatial layer and all the ones below it, in all of them
			// up to the given temporal layer. A temporal layer above the given one is
			// left out of the lower spatial layers too, since all the frames of a
			// picture share its temporal layer and hence are dropped together.
			for (uint8_t sIdx{ 0 }; sIdx <= spatialLayer; ++sIdx)
			{
				for (uint8_t tIdx{ 0 }; tIdx <= temporalLayer; ++tIdx)
				{
					auto& temporalLayerCounter = this->spatialLayerCounters[sIdx][tIdx];

					rate += temporalLayerCounter.GetBitrate(nowMs).value_or(0);
				}
			}

			return rate;
		}

		int64_t RtpStreamRecv::TransmissionCounter::GetSpatialLayerBitrate(int64_t nowMs, uint8_t spatialLayer)
		{
			MS_TRACE();

			MS_ASSERT(spatialLayer < this->spatialLayerCounters.size(), "spatialLayer too high");

			int64_t rate{ 0 };

			for (size_t tIdx{ 0 }; tIdx < this->spatialLayerCounters[spatialLayer].size(); ++tIdx)
			{
				auto& temporalLayerCounter = this->spatialLayerCounters[spatialLayer][tIdx];

				rate += temporalLayerCounter.GetBitrate(nowMs).value_or(0);
			}

			return rate;
		}

		int64_t RtpStreamRecv::TransmissionCounter::GetCumulativeTemporalLayerBitrate(
		  int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer)
		{
			MS_TRACE();

			MS_ASSERT(spatialLayer < this->spatialLayerCounters.size(), "spatialLayer too high");
			MS_ASSERT(
			  temporalLayer < this->spatialLayerCounters[spatialLayer].size(), "temporalLayer too high");

			int64_t rate{ 0 };

			for (uint8_t tIdx{ 0 }; tIdx <= temporalLayer; ++tIdx)
			{
				auto& temporalLayerCounter = this->spatialLayerCounters[spatialLayer][tIdx];

				rate += temporalLayerCounter.GetBitrate(nowMs).value_or(0);
			}

			return rate;
		}

		int64_t RtpStreamRecv::TransmissionCounter::GetTemporalLayerBitrate(
		  int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer)
		{
			MS_TRACE();

			MS_ASSERT(spatialLayer < this->spatialLayerCounters.size(), "spatialLayer too high");
			MS_ASSERT(
			  temporalLayer < this->spatialLayerCounters[spatialLayer].size(), "temporalLayer too high");

			auto& temporalLayerCounter = this->spatialLayerCounters[spatialLayer][temporalLayer];

			return temporalLayerCounter.GetBitrate(nowMs).value_or(0);
		}

		uint64_t RtpStreamRecv::TransmissionCounter::GetPacketCount() const
		{
			MS_TRACE();

			uint64_t packetCount{ 0 };

			for (const auto& spatialLayerCounter : this->spatialLayerCounters)
			{
				for (const auto& temporalLayerCounter : spatialLayerCounter)
				{
					packetCount += temporalLayerCounter.GetPacketCount();
				}
			}

			return packetCount;
		}

		uint64_t RtpStreamRecv::TransmissionCounter::GetBytes() const
		{
			MS_TRACE();

			uint64_t bytes{ 0 };

			for (const auto& spatialLayerCounter : this->spatialLayerCounters)
			{
				for (const auto& temporalLayerCounter : spatialLayerCounter)
				{
					bytes += temporalLayerCounter.GetBytes();
				}
			}

			return bytes;
		}

		/* Instance methods. */

		RtpStreamRecv::RtpStreamRecv(
		  RTP::RtpStreamRecv::Listener* listener,
		  SharedInterface* shared,
		  RTP::RtpStream::Params& params,
		  int64_t sendNackDelayMs,
		  bool useRtpInactivityCheck)
		  : RTP::RtpStream::RtpStream(listener, shared, params, 10),
		    sendNackDelayMs(sendNackDelayMs),
		    useRtpInactivityCheck(useRtpInactivityCheck),
		    inactivityCheckPeriodicTimer(
		      useRtpInactivityCheck ? shared->CreateTimer(this, "rtp-stream-recv-inactivity-check")
					                      : nullptr),
		    spatialLayersActivityCheckPeriodicTimer(
		      GetSpatialLayers() > 1
		        ? shared->CreateTimer(this, "rtp-stream-recv-spatial-layers-activity-check")
						: nullptr),
		    transmissionCounter(
		      shared, params.spatialLayers, params.temporalLayers, this->params.useDtx ? 6000 : 2500),
		    mediaTransmissionCounter(shared, /*ignorePaddingOnlyPackets*/ true)
		{
			MS_TRACE();

			if (this->params.useNack)
			{
				this->nackGenerator.reset(new RTC::NackGenerator(this, this->shared, this->sendNackDelayMs));
			}

			this->inactive = false;

			if (this->useRtpInactivityCheck)
			{
				// Run the RTP inactivity periodic timer (use a different timeout if DTX is
				// enabled).
				this->inactivityCheckPeriodicTimer->Start(
				  this->params.useDtx ? InactivityCheckIntervalWithDtxMs : InactivityCheckIntervalMs);
			}

			// A stream with more than a spatial layer carries all of them within the
			// same SSRC, so the inactivity check above cannot tell that just one of
			// them stopped. Watch their traffic separately.
			if (GetSpatialLayers() > 1)
			{
				this->spatialLayersActivity.assign(GetSpatialLayers(), false);

				this->spatialLayersActivityCheckPeriodicTimer->Start(
				  SpatialLayersActivityCheckIntervalMs, SpatialLayersActivityCheckIntervalMs);
			}
		}

		RtpStreamRecv::~RtpStreamRecv()
		{
			MS_TRACE();
		}

		flatbuffers::Offset<FBS::RtpStream::Stats> RtpStreamRecv::FillBufferStats(
		  flatbuffers::FlatBufferBuilder& builder)
		{
			MS_TRACE();

			const int64_t nowMs = this->shared->GetTimeMs();

			const auto baseStats = RTP::RtpStream::FillBufferStats(builder);

			std::vector<flatbuffers::Offset<FBS::RtpStream::BitrateByLayer>> bitrateByLayer;

			if (GetSpatialLayers() > 1 || GetTemporalLayers() > 1)
			{
				for (uint8_t sIdx = 0; sIdx < GetSpatialLayers(); ++sIdx)
				{
					for (uint8_t tIdx = 0; tIdx < GetTemporalLayers(); ++tIdx)
					{
						const auto layer = std::to_string(sIdx) + "." + std::to_string(tIdx);

						// In K-SVC a spatial layer is decoded on its own, so what running
						// at these layers costs is that layer alone up to this temporal
						// one. Everywhere else the layers below go along with it.
						const int64_t bitrate = IsKSvc() ? GetCumulativeTemporalLayerBitrate(nowMs, sIdx, tIdx)
						                                 : GetCumulativeBitrate(nowMs, sIdx, tIdx);

						bitrateByLayer.emplace_back(
						  FBS::RtpStream::CreateBitrateByLayerDirect(
						    builder, layer.c_str(), static_cast<uint64_t>(bitrate)));
					}
				}
			}

			const auto stats = FBS::RtpStream::CreateRecvStatsDirect(
			  builder,
			  baseStats,
			  this->transmissionCounter.GetPacketCount(),
			  this->transmissionCounter.GetBytes(),
			  static_cast<uint64_t>(this->transmissionCounter.GetBitrate(nowMs)),
			  std::addressof(bitrateByLayer));

			return FBS::RtpStream::CreateStats(builder, FBS::RtpStream::StatsData::RecvStats, stats.Union());
		}

		bool RtpStreamRecv::ReceivePacket(RTP::Packet* packet, int64_t receivedAtUs)
		{
			MS_TRACE();

			// Whether this is the first packet of the stream, and the extended highest
			// sequence number before it, to tell whether it moves the stream forward.
			const bool isFirstPacket          = !IsStarted();
			const uint32_t prevExtendedMaxSeq = this->cycles + this->maxSeq;

			// Call the parent method.
			if (!RTP::RtpStream::ReceiveStreamPacket(packet))
			{
				MS_WARN_TAG(rtp, "packet discarded");

				return false;
			}

			// A requested retransmission that arrives this late is not forwarded, since
			// the streams of the Consumers would take it as invalid, but the
			// NackGenerator is told so that it stops requesting it.
			if (IsTooOldToForward(packet))
			{
				if (this->params.useNack)
				{
					this->nackGenerator->ReceivePacket(packet, /*isRecovered*/ false);
				}

				return false;
			}

			// Process the packet at codec level.
			if (packet->GetPayloadType() == GetPayloadType())
			{
				RTP::Codecs::Tools::ProcessRtpPacket(packet, GetMimeType(), this->dependencyDescriptorContext);
			}

			// Tell the listener if this packet made the stream restart, now that it
			// can tell whether the packet starts a key frame.
			if (this->restarted)
			{
				this->restarted = false;

				static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)->OnRtpStreamRestarted(this, packet);
			}

			// Pass the packet to the NackGenerator.
			if (this->params.useNack)
			{
				// If there is RTX just provide the NackGenerator with the packet.
				if (HasRtx())
				{
					this->nackGenerator->ReceivePacket(packet, /*isRecovered*/ false);
				}
				// If there is no RTX and NackGenerator returns true it means that it
				// was a NACKed packet.
				else if (this->nackGenerator->ReceivePacket(packet, /*isRecovered*/ false))
				{
					// Mark the packet as retransmitted and repaired.
					RTP::RtpStream::PacketRetransmitted(packet);
					RTP::RtpStream::PacketRepaired(packet);
				}
			}

			// Late packets (reordered or retransmitted) say nothing about the current
			// state of the stream.
			const bool movesStreamForward =
			  isFirstPacket || this->cycles + this->maxSeq != prevExtendedMaxSeq;

			// Calculate jitter, just with packets that move the stream forward, since
			// late ones would inflate it.
			if (movesStreamForward)
			{
				CalculateJitter(packet->GetTimestamp(), receivedAtUs);
			}

			// Store the capture instant if the packet carries it, just with packets
			// that move the stream forward, since late ones would replace a newer one.
			if (movesStreamForward)
			{
				uint64_t absCaptureTimestamp{ 0 };
				int64_t estimatedCaptureClockOffset{ 0 };

				// NOTE: A zeroed capture timestamp gives no capture instant to store.
				if (packet->ReadAbsCaptureTime(absCaptureTimestamp, estimatedCaptureClockOffset) && absCaptureTimestamp != 0)
				{
					Utils::Time::Ntp ntp{}; // NOLINT(cppcoreguidelines-pro-type-member-init)

					ntp.seconds   = static_cast<uint32_t>(absCaptureTimestamp >> 32);
					ntp.fractions = static_cast<uint32_t>(absCaptureTimestamp);

					// The capture timestamp belongs to the clock of the capture system, which is
					// not the clock of the sender of this stream when the media comes from
					// somewhere else. The capture clock offset gives the instant back in the clock
					// of the sender, which is the one this stream is estimated against:
					//
					//   capture NTP clock = sender NTP clock + capture clock offset
					const auto captureAtUs =
					  Utils::Time::NtpToTimeUs(ntp) - Utils::Time::Q32x32ToTimeUs(estimatedCaptureClockOffset);

					// NOTE: A capture instant that is not positive means a capture clock offset
					// that cannot be true, so there is nothing to store.
					if (captureAtUs > 0)
					{
						this->lastAbsCaptureTime = AbsCaptureTime{
							.ts    = packet->GetTimestamp(),
							.ntpUs = captureAtUs,
						};
					}
				}
			}

			// Increase transmission counter.
			this->transmissionCounter.Update(packet);

			// Increase media transmission counter.
			this->mediaTransmissionCounter.Update(packet);

			this->lastPacketReceivedAtUs = receivedAtUs;

			// Padding only packet, do not consider it for stream activation.
			if (packet->GetPayloadLength() == 0)
			{
				return true;
			}

			// Not inactive anymore.
			if (this->inactive)
			{
				this->inactive = false;

				ResetScore(10, /*notify*/ true);
			}

			// Restart the inactivityCheckPeriodicTimer.
			if (this->inactivityCheckPeriodicTimer)
			{
				this->inactivityCheckPeriodicTimer->Restart();
			}

			return true;
		}

		bool RtpStreamRecv::ReceiveRtxPacket(RTP::Packet* packet, int64_t receivedAtUs)
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

			if (HasRtx())
			{
				if (!this->rtxStream->ReceivePacket(packet, receivedAtUs))
				{
					MS_WARN_TAG(rtx, "RTX packet discarded");

					return false;
				}
			}

#if MS_LOG_DEV_LEVEL == 3
			// Get the RTX packet sequence number for logging purposes.
			const auto rtxSeq = packet->GetSequenceNumber();
#endif

			// Get the original RTP packet.
			if (!packet->RtxDecode(this->params.payloadType, this->params.ssrc))
			{
				MS_DEBUG_DEV(
				  "ignoring empty RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 ", pt:%" PRIu8 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber(),
				  packet->GetPayloadType());

				return false;
			}

			MS_DEBUG_DEV(
			  "received RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "] recovering original [ssrc:%" PRIu32
			  ", seq:%" PRIu16 "]",
			  this->params.rtxSsrc,
			  rtxSeq,
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			// If not a valid packet ignore it.
			if (!RTP::RtpStream::UpdateRecoveredSeq(packet))
			{
				MS_WARN_TAG(
				  rtx,
				  "invalid RTX packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());

				return false;
			}

			// A requested retransmission that arrives this late is not forwarded, since
			// the streams of the Consumers would take it as invalid, but the
			// NackGenerator is told so that it stops requesting it.
			if (IsTooOldToForward(packet))
			{
				this->nackGenerator->ReceivePacket(packet, /*isRecovered*/ true);

				return false;
			}

			// Process the packet at codec level.
			if (packet->GetPayloadType() == GetPayloadType())
			{
				RTP::Codecs::Tools::ProcessRtpPacket(packet, GetMimeType(), this->dependencyDescriptorContext);
			}

			// Mark the packet as retransmitted.
			RTP::RtpStream::PacketRetransmitted(packet);

			// Pass the packet to the NackGenerator and return true just if this was a
			// NACKed packet or a RTX packet containing a non yet seen original RTP
			// packet.
			if (this->nackGenerator->ReceivePacket(packet, /*isRecovered*/ true))
			{
				// Mark the packet as repaired.
				RTP::RtpStream::PacketRepaired(packet);

				// Increase transmission counter.
				this->transmissionCounter.Update(packet);

				// Padding only packet, do not consider it for stream activation.
				if (packet->GetPayloadLength() == 0)
				{
					return true;
				}

				// Not inactive anymore.
				if (this->inactive)
				{
					this->inactive = false;

					ResetScore(10, /*notify*/ true);
				}

				// Restart the inactivityCheckPeriodicTimer.
				if (this->inactivityCheckPeriodicTimer)
				{
					this->inactivityCheckPeriodicTimer->Restart();
				}

				return true;
			}

			return false;
		}

		bool RtpStreamRecv::AddRtcpReportBlock(
		  RTC::RTCP::ReceiverReportPacket* receiverReportPacket, int64_t nowUs)
		{
			MS_TRACE();

			if (!HasRtcpReportBlock(nowUs))
			{
				return false;
			}

			if (receiverReportPacket->GetNumberOfReportBlocks() == RTC::RTCP::AnyReportPacket::MaxReportBlocks)
			{
				return false;
			}

			if (receiverReportPacket->GetAvailableLength() < RTC::RTCP::AnyReportPacket::ReportBlockLength)
			{
				return false;
			}

			// Ask the listener for the worst remote fraction lost.
			const uint8_t worstRemoteFractionLost =
			  this->params.useInBandFec ? static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)
			                                ->OnRtpStreamNeedWorstRemoteFractionLost(this)
				                          : 0;

			if (worstRemoteFractionLost > 0)
			{
				MS_DEBUG_TAG(rtcp, "using worst remote fraction lost:%" PRIu8, worstRemoteFractionLost);
			}

			RTC::RTCP::AnyReportPacket::ReportBlock reportBlock{ .ssrc = GetSsrc() };

			const int32_t prevPacketsLost = this->packetsLost;

			// Calculate packets expected and lost.
			const auto expectedPackets = GetExpectedPackets();

			// NOTE: The expected count is the extended sequence number arithmetic of RFC
			// 3550, so it wraps at 32 bits, whereas the received one does not wrap at
			// all. Each subtraction below is therefore made in the width that keeps it
			// right: this one truncates the received count so that both wrap together,
			// and the interval further down is taken in full width, where it is exact.
			const auto receivedPackets =
			  static_cast<uint32_t>(this->mediaTransmissionCounter.GetPacketCount());

			if (expectedPackets > receivedPackets)
			{
				this->packetsLost = static_cast<int32_t>(expectedPackets - receivedPackets);
			}
			else
			{
				this->packetsLost = 0;
			}

			// Calculate fraction lost.
			//
			// NOTE: The expected count wraps at 32 bits, so the difference is taken in
			// that width.
			const int64_t expectedInterval = static_cast<int32_t>(expectedPackets - this->expectedPrior);

			this->expectedPrior = expectedPackets;

			const auto receivedInterval =
			  static_cast<int64_t>(this->mediaTransmissionCounter.GetPacketCount() - this->receivedPrior);

			this->receivedPrior = this->mediaTransmissionCounter.GetPacketCount();

			const int64_t lostInterval = expectedInterval - receivedInterval;

			if (expectedInterval <= 0 || lostInterval <= 0)
			{
				this->fractionLost = 0;
			}
			else
			{
				// A fixed point number with 8 bits of fraction, so a whole interval lost
				// gives 256, one more than the field can hold.
				this->fractionLost =
				  static_cast<uint8_t>(std::min<int64_t>((lostInterval << 8) / expectedInterval, 255));
			}

			// Worst remote fraction lost is not worse than local one.
			if (worstRemoteFractionLost <= this->fractionLost)
			{
				this->reportedPacketsLost += (this->packetsLost - prevPacketsLost);

				reportBlock.totalLost    = this->reportedPacketsLost;
				reportBlock.fractionLost = this->fractionLost;
			}
			else
			{
				// Recalculate packetsLost.
				//
				// NOTE: Clamped as the fraction lost above, since the difference read as
				// signed may come out negative.
				const auto newLostInterval = static_cast<uint32_t>(
				  (worstRemoteFractionLost * std::max<int64_t>(expectedInterval, 0)) >> 8);

				this->reportedPacketsLost += newLostInterval;

				reportBlock.totalLost    = this->reportedPacketsLost;
				reportBlock.fractionLost = worstRemoteFractionLost;
			}

			// Fill the rest of the report block.
			reportBlock.extendedHighestSeq = static_cast<uint32_t>(this->maxSeq) + this->cycles;
			reportBlock.jitter             = this->jitter;

			if (this->lastSenderReportTiming.has_value())
			{
				const auto& senderReportTiming = this->lastSenderReportTiming.value();

				// Get delay in microseconds.
				const int64_t delayUs = nowUs - senderReportTiming.receivedAtUs;

				// Express delay in units of 1/65536 seconds.
				auto dlsr = static_cast<uint32_t>((delayUs / 1000000) << 16);

				dlsr |= static_cast<uint32_t>(((delayUs % 1000000) * 65536) / 1000000);

				reportBlock.delaySinceLastSenderReport = dlsr;
				reportBlock.lastSenderReport           = senderReportTiming.compactNtp;
			}
			else
			{
				reportBlock.delaySinceLastSenderReport = 0;
				reportBlock.lastSenderReport           = 0;
			}

			receiverReportPacket->AddReportBlock(reportBlock);

			return true;
		}

		bool RtpStreamRecv::AddRtxRtcpReportBlock(
		  RTC::RTCP::ReceiverReportPacket* receiverReportPacket, int64_t nowUs)
		{
			MS_TRACE();

			if (HasRtx())
			{
				return this->rtxStream->AddRtcpReportBlock(receiverReportPacket, nowUs);
			}

			return false;
		}

		void RtpStreamRecv::ReceiveRtcpSenderReportPacket(
		  const RTC::RTCP::SenderReportPacket* senderReportPacket, int64_t receivedAtUs)
		{
			MS_TRACE();

			const Utils::Time::Ntp ntp = senderReportPacket->GetNtp();

			// Update info about last Sender Report.
			//
			// NOTE: A sender with no wall clock may report a zeroed NTP timestamp, which
			// gives nothing to store.
			if (ntp.seconds != 0 || ntp.fractions != 0)
			{
				this->lastSenderReportTiming = SenderReportTiming{
					.compactNtp   = Utils::Time::NtpToCompactNtp(ntp),
					.receivedAtUs = receivedAtUs,
				};

				this->lastSenderReportMapping = RTP::RtpStream::SenderReportMapping{
					.ntpUs = Utils::Time::NtpToTimeUs(ntp),
					.ts    = senderReportPacket->GetRtpTimestamp(),
				};
			}

			// Update the score.
			UpdateScore();
		}

		void RtpStreamRecv::ReceiveRtxRtcpSenderReportPacket(
		  const RTC::RTCP::SenderReportPacket* senderReportPacket, int64_t receivedAtUs)
		{
			MS_TRACE();

			if (HasRtx())
			{
				this->rtxStream->ReceiveRtcpSenderReportPacket(senderReportPacket, receivedAtUs);
			}
		}

		std::optional<float> RtpStreamRecv::ReceiveRtcpDlrrSubBlock(
		  const RTC::RTCP::DlrrExtendedReport::SubBlock& subBlock, int64_t receivedAtUs)
		{
			MS_TRACE();

			/* Calculate RTT. */

			const uint32_t lastRr = subBlock.lastReceiverReport;
			const uint32_t dlrr   = subBlock.delaySinceLastReceiverReport;

			// If the remote endpoint has not received any Receiver Reference Time
			// Extended Report from us yet, the DLRR sub-block carries no RTT, so the
			// last one is kept and nothing new is reported.
			if (lastRr == 0)
			{
				return std::nullopt;
			}

			// Get the NTP representation of the time at which the report arrived, which
			// is what the round trip is measured against.
			const Utils::Time::Ntp ntp =
			  Utils::Time::TimeUsToNtp(receivedAtUs + this->shared->GetNtpOffsetUs());

			// Get the compact NTP representation of the arrival time.
			const uint32_t compactNtp = Utils::Time::NtpToCompactNtp(ntp);

			// NOTE: The subtraction wraps around along with the compact NTP
			// representation, which is what the conversion expects.
			const float measuredRttMs =
			  static_cast<float>(Utils::Time::CompactNtpRttToTimeUs(compactNtp - dlrr - lastRr)) / 1000;

			this->rttMs = measuredRttMs;

			return measuredRttMs;
		}

		void RtpStreamRecv::UpdateRttMs(int64_t rttMs)
		{
			MS_TRACE();

			if (this->params.useNack)
			{
				this->nackGenerator->UpdateRttMs(rttMs);
			}
		}

		std::optional<int64_t> RtpStreamRecv::GetRemoteCaptureAtUsFromAbsCaptureTime(uint32_t ts) const
		{
			MS_TRACE();

			if (!this->lastAbsCaptureTime.has_value())
			{
				return std::nullopt;
			}

			const auto& absCaptureTime = this->lastAbsCaptureTime.value();

			return InterpolateRemoteCaptureAtUs(
			  absCaptureTime.ntpUs,
			  absCaptureTime.ts,
			  ts,
			  RtpStreamRecv::MaxAbsCaptureTimeInterpolationMs * 1000);
		}

		std::optional<int64_t> RtpStreamRecv::GetRemoteCaptureAtUsFromSenderReport(uint32_t ts) const
		{
			MS_TRACE();

			if (!this->lastSenderReportMapping.has_value())
			{
				return std::nullopt;
			}

			const auto& senderReportMapping = this->lastSenderReportMapping.value();

			return InterpolateRemoteCaptureAtUs(
			  senderReportMapping.ntpUs,
			  senderReportMapping.ts,
			  ts,
			  RtpStreamRecv::MaxSenderReportInterpolationMs * 1000);
		}

		std::optional<int64_t> RtpStreamRecv::InterpolateRemoteCaptureAtUs(
		  int64_t referenceNtpUs, uint32_t referenceTs, uint32_t ts, int64_t maxDistanceUs) const
		{
			MS_TRACE();

			const auto clockRate = GetClockRate();

			if (clockRate == 0)
			{
				return std::nullopt;
			}

			// Distance in RTP timestamp units, taking wrap around into account.
			const auto distanceTs    = static_cast<int64_t>(static_cast<int32_t>(ts - referenceTs));
			const int64_t distanceUs = (distanceTs * 1000000) / static_cast<int64_t>(clockRate);
			// NOTE: The negation is safe since `distanceUs` comes from a 32 bits
			// distance scaled down by the clock rate.
			const int64_t absDistanceUs = distanceUs < 0 ? -distanceUs : distanceUs;

			if (absDistanceUs > maxDistanceUs)
			{
				return std::nullopt;
			}

			const int64_t captureAtUs = referenceNtpUs + distanceUs;

			// The reference does not map into a valid capture instant, so the remote
			// endpoint is reporting nonsense.
			if (captureAtUs < 0)
			{
				MS_WARN_2TAGS(
				  rtp, rtcp, "invalid interpolated capture instant [distanceUs:%" PRIi64 "]", distanceUs);

				return std::nullopt;
			}

			return captureAtUs;
		}

		void RtpStreamRecv::RequestKeyFrame()
		{
			MS_TRACE();

			if (this->params.usePli)
			{
				MS_DEBUG_2TAGS(rtcp, rtx, "sending PLI [ssrc:%" PRIu32 "]", GetSsrc());

				const std::unique_ptr<RTC::RTCP::PliPacket> pliPacket{ RTC::RTCP::PliPacket::Factory(
					RtcpPacketBuffer, sizeof(RtcpPacketBuffer)) };

				pliPacket->SetSenderSsrc(this->params.rtcpSenderSsrc);
				pliPacket->SetMediaSsrc(GetSsrc());

				this->pliCount++;

				// Notify the listener.
				static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)
				  ->OnRtpStreamSendRtcpPacket(this, pliPacket.get());
			}
			else if (this->params.useFir)
			{
				MS_DEBUG_2TAGS(rtcp, rtx, "sending FIR [ssrc:%" PRIu32 "]", GetSsrc());

				const std::unique_ptr<RTC::RTCP::FirPacket> firPacket{ RTC::RTCP::FirPacket::Factory(
					RtcpPacketBuffer, sizeof(RtcpPacketBuffer)) };

				firPacket->SetSenderSsrc(this->params.rtcpSenderSsrc);
				firPacket->SetMediaSsrc(GetSsrc());
				firPacket->AddItem(GetSsrc(), ++this->firSeqNumber);

				this->firCount++;

				// Notify the listener.
				static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)
				  ->OnRtpStreamSendRtcpPacket(this, firPacket.get());
			}
		}

		void RtpStreamRecv::Pause()
		{
			MS_TRACE();

			if (this->inactivityCheckPeriodicTimer)
			{
				this->inactivityCheckPeriodicTimer->Stop();
			}

			if (this->params.useNack)
			{
				this->nackGenerator->Pause();
			}

			// Reset jitter.
			this->transit = 0;
			this->jitter  = 0;
		}

		void RtpStreamRecv::Resume()
		{
			MS_TRACE();

			if (this->inactivityCheckPeriodicTimer && !this->inactive)
			{
				this->inactivityCheckPeriodicTimer->Restart();
			}
		}

		bool RtpStreamRecv::IsTooOldToForward(const RTP::Packet* packet) const
		{
			MS_TRACE();

			const uint16_t udelta = packet->GetSequenceNumber() - this->maxSeq;

			// Behind the highest sequence number seen, and as far as `UpdateSeq()`
			// takes as a bad sequence number.
			return udelta > RTP::RtpStream::RtpSeqMod / 2 &&
			       udelta <= RTP::RtpStream::RtpSeqMod - RTP::RtpStream::MaxMisorder;
		}

		void RtpStreamRecv::CalculateJitter(uint32_t rtpTimestamp, int64_t receivedAtUs)
		{
			MS_TRACE();

			if (GetClockRate() == 0)
			{
				return;
			}

			// The arrival time expressed in the clock rate of the stream, which is
			// the R of RFC 3550 section 6.4.1.
			//
			// NOTE: The seconds and the sub-second parts are converted separately
			// because a single multiplication of the whole time by the clock rate
			// would overflow on a long lived host.
			const auto arrivalTs =
			  static_cast<uint32_t>((receivedAtUs / 1000000) * GetClockRate()) +
			  static_cast<uint32_t>(((receivedAtUs % 1000000) * GetClockRate()) / 1000000);

			// NOTE: Based on https://github.com/versatica/mediasoup/issues/1018.
			const auto transit = static_cast<int32_t>(arrivalTs - rtpTimestamp);

			// NOTE: Wider than its operands because the difference of two int32_t does
			// not fit in an int32_t, and neither would negating its lowest value.
			int64_t d = static_cast<int64_t>(transit) - this->transit;

			// First transit calculation, save and return.
			if (this->transit == 0)
			{
				this->transit = transit;

				return;
			}

			this->transit = transit;

			if (d < 0)
			{
				d = -d;
			}

			// A jump of the RTP timestamp, such as the one of a sender that restarts
			// its stream, says nothing about the network, so it is left out.
			if (d >= MinIgnoredJitterTransitDifference)
			{
				return;
			}

			this->jitter += (1. / 16.) * (static_cast<float>(d) - this->jitter);
		}

		void RtpStreamRecv::UpdateScore()
		{
			MS_TRACE();

			// Calculate the packets of this interval, which is what each counter has
			// grown by since the previous score update.
			const auto totalExpectedPackets      = GetExpectedPackets();
			const auto totalReceivedPackets      = this->mediaTransmissionCounter.GetPacketCount();
			const auto totalRepairedPackets      = this->packetsRepaired;
			const auto totalRetransmittedPackets = this->packetsRetransmitted;
			const uint32_t expectedPackets       = totalExpectedPackets - this->expectedPriorScore;
			const auto receivedPackets           = totalReceivedPackets - this->receivedPriorScore;

			uint64_t lostPackets =
			  expectedPackets < receivedPackets ? 0 : expectedPackets - receivedPackets;
			auto repairedPackets      = totalRepairedPackets - this->repairedPriorScore;
			auto retransmittedPackets = totalRetransmittedPackets - this->retransmittedPriorScore;

			this->expectedPriorScore      = totalExpectedPackets;
			this->receivedPriorScore      = totalReceivedPackets;
			this->repairedPriorScore      = totalRepairedPackets;
			this->retransmittedPriorScore = totalRetransmittedPackets;

			if (this->inactive)
			{
				return;
			}

			// We didn't expect more packets to come.
			if (expectedPackets == 0)
			{
				RTP::RtpStream::UpdateScore(10);

				return;
			}

			// We expected packets but received none of them, so there is nothing to
			// compute (and ratios below would divide by zero).
			if (receivedPackets == 0)
			{
				RTP::RtpStream::UpdateScore(0);

				return;
			}

			lostPackets = std::min(lostPackets, receivedPackets);

			if (repairedPackets > lostPackets)
			{
				if (HasRtx())
				{
					// NOTE: The excess has to be discounted before clamping, since once
					// `repairedPackets` has been clamped there is no excess left to tell.
					retransmittedPackets -= repairedPackets - lostPackets;
					repairedPackets = lostPackets;
				}
				else
				{
					lostPackets = repairedPackets;
				}
			}

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[totalExpectedPackets:%" PRIu32 ", totalReceivedPackets:%" PRIu64
			  ", totalRepairedPackets:%" PRIu64,
			  totalExpectedPackets,
			  totalReceivedPackets,
			  totalRepairedPackets);

			MS_DEBUG_TAG(
			  score,
			  "fixed values [expectedPackets:%" PRIu32 ", receivedPackets:%" PRIu64
			  ", lostPackets:%" PRIu64 ", repairedPackets:%" PRIu64 ", retransmittedPackets:%" PRIu64,
			  expectedPackets,
			  receivedPackets,
			  lostPackets,
			  repairedPackets,
			  retransmittedPackets);
#endif

			const auto repairedRatio =
			  static_cast<float>(repairedPackets) / static_cast<float>(receivedPackets);

			auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

			MS_ASSERT(
			  retransmittedPackets >= repairedPackets,
			  "repaired packets cannot be more than retransmitted ones");

			if (retransmittedPackets > 0)
			{
				repairedWeight *= static_cast<float>(repairedPackets) / retransmittedPackets;
			}

			lostPackets = static_cast<uint64_t>(lostPackets - (repairedPackets * repairedWeight));

			const auto deliveredRatio =
			  static_cast<float>(receivedPackets - lostPackets) / static_cast<float>(receivedPackets);
			const auto score = static_cast<uint8_t>(std::round(std::pow(deliveredRatio, 4) * 10));

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[deliveredRatio:%f, repairedRatio:%f, repairedWeight:%f, new lostPackets:%" PRIu64
			  ", score:%" PRIu8 "]",
			  deliveredRatio,
			  repairedRatio,
			  repairedWeight,
			  lostPackets,
			  score);
#endif

			// Call the parent method for update score.
			RTP::RtpStream::UpdateScore(score);
		}

		void RtpStreamRecv::UserOnSequenceNumberReset()
		{
			MS_TRACE();

			// The RTP timestamps of a restarted stream have nothing to do with the
			// previous ones, so the next packet starts the transit calculation again.
			this->transit = 0;

			// The sequence numbers of a restarted stream have nothing to do with the
			// previous ones either, so the next packet starts the NACK generation
			// again.
			if (this->params.useNack)
			{
				this->nackGenerator->Reset();
			}

			// Whether it needs a key frame can only be told once the packet has been
			// processed at codec level, so the listener is told later.
			this->restarted = true;
		}

		bool RtpStreamRecv::IsRequestedRetransmission(const RTP::Packet* packet) const
		{
			MS_TRACE();

			return this->params.useNack && this->nackGenerator->IsNacked(packet->GetSequenceNumber());
		}

		inline void RtpStreamRecv::OnTimer(TimerHandleInterface* timer)
		{
			MS_TRACE();

			if (timer == this->inactivityCheckPeriodicTimer.get())
			{
				this->inactive = true;

				if (GetScore() != 0)
				{
					MS_WARN_2TAGS(
					  rtp, score, "RTP inactivity detected, resetting score to 0 [ssrc:%" PRIu32 "]", GetSsrc());
				}

				ResetScore(0, /*notify*/ true);
			}
			else if (timer == this->spatialLayersActivityCheckPeriodicTimer.get())
			{
				const int64_t nowMs = this->shared->GetTimeMs();

				for (uint8_t spatialLayer{ 0 }; spatialLayer < GetSpatialLayers(); ++spatialLayer)
				{
					const bool isActive =
					  this->transmissionCounter.GetSpatialLayerBitrate(nowMs, spatialLayer) > 0;

					if (isActive == this->spatialLayersActivity[spatialLayer])
					{
						continue;
					}

					this->spatialLayersActivity[spatialLayer] = isActive;

					MS_DEBUG_2TAGS(
					  rtp,
					  svc,
					  "spatial layer activity changed [ssrc:%" PRIu32 ", spatialLayer:%" PRIu8 ", isActive:%s]",
					  GetSsrc(),
					  spatialLayer,
					  isActive ? "true" : "false");

					static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)
					  ->OnRtpStreamSpatialLayerActivityChanged(this, spatialLayer, isActive);
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

			std::unique_ptr<RTC::RTCP::NackPacket> nackPacket{ RTC::RTCP::NackPacket::Factory(
				RtcpPacketBuffer, sizeof(RtcpPacketBuffer)) };

			nackPacket->SetSenderSsrc(this->params.rtcpSenderSsrc);
			nackPacket->SetMediaSsrc(GetSsrc());

			size_t numPacketsRequested{ 0 };

			// Sends the NACK packet built so far.
			const auto sendPacket = [this, &nackPacket, &numPacketsRequested]() -> void
			{
				this->nackCount++;
				this->nackPacketCount += numPacketsRequested;

				// Notify the listener.
				static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)
				  ->OnRtpStreamSendRtcpPacket(this, nackPacket.get());
			};

			auto it        = seqNumbers.begin();
			const auto end = seqNumbers.end();

			while (it != end)
			{
				const uint16_t seq = *it;
				uint16_t bitmask{ 0 };

				++it;

				while (it != end)
				{
					const uint16_t shift = *it - seq - 1;

					if (shift > 15)
					{
						break;
					}

					bitmask |= (1 << shift);
					++it;
				}

				// If the item does not fit in the packet, send the packet and start a
				// new one.
				if (nackPacket->GetAvailableLength() < RTC::RTCP::NackPacket::NackItemLength)
				{
					sendPacket();

					nackPacket.reset(RTC::RTCP::NackPacket::Factory(RtcpPacketBuffer, sizeof(RtcpPacketBuffer)));

					nackPacket->SetSenderSsrc(this->params.rtcpSenderSsrc);
					nackPacket->SetMediaSsrc(GetSsrc());

					numPacketsRequested = 0;
				}

				nackPacket->AddItem(seq, bitmask);

				// The item requests the packet with its sequence number plus one for
				// every bit set in its bitmask.
				numPacketsRequested += Utils::Bits::CountSetBits(bitmask) + 1;
			}

			sendPacket();
		}

		inline void RtpStreamRecv::OnNackGeneratorKeyFrameRequired()
		{
			MS_TRACE();

			MS_DEBUG_TAG(rtx, "requesting key frame [ssrc:%" PRIu32 "]", this->params.ssrc);

			// Notify the listener.
			static_cast<RTP::RtpStreamRecv::Listener*>(this->listener)->OnRtpStreamKeyFrameRequired(this);
		}
	} // namespace RTP
} // namespace RTC
