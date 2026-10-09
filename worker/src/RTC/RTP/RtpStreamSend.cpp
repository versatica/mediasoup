#define MS_CLASS "RTC::RTP::RtpStreamSend"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/RtpStreamSend.hpp"
#include "Logger.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include <cmath> // std::pow(), std::round()
#include <limits>

namespace RTC
{
	namespace RTP
	{
		/* Static. */

		// Limit max number of items in the retransmission buffer.
		static constexpr size_t RetransmissionBufferMaxItems{ 3000 };
		static constexpr int64_t DefaultRttMs{ 100 };
		// Interval between consecutive iterations of pending retransmissions.
		static constexpr int64_t RetransmissionIntervalMs{ 10 };
		// Maximum number of packets retransmitted in each iteration.
		static constexpr size_t MaxRetransmittedPacketsPerIteration{ 2 };

		/* Instance methods. */

		RtpStreamSend::RtpStreamSend(
		  RTP::RtpStreamSend::Listener* listener,
		  SharedInterface* shared,
		  RTP::RtpStream::Params& params,
		  std::string& mid)
		  : RTP::RtpStream::RtpStream(listener, shared, params, 10),
		    mid(mid),
		    transmissionCounter(shared, /*ignorePaddingOnlyPackets*/ true),
		    retransmissionTimer(
		      params.useNack ? shared->CreateTimer(this, "rtp-stream-send-retransmissions") : nullptr)
		{
			MS_TRACE();

			if (this->params.useNack)
			{
				int64_t maxRetransmissionDelayMs{ 0 };

				switch (params.mimeType.type)
				{
					case RTC::RtpCodecMimeType::Type::VIDEO:
					{
						maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForVideoMs;

						break;
					}

					case RTC::RtpCodecMimeType::Type::AUDIO:
					{
						maxRetransmissionDelayMs = RtpStreamSend::MaxRetransmissionDelayForAudioMs;

						break;
					}
				}

				this->retransmissionBuffer = new RTC::RTP::RetransmissionBuffer(
				  RetransmissionBufferMaxItems, maxRetransmissionDelayMs, params.clockRate);
			}
		}

		RtpStreamSend::~RtpStreamSend()
		{
			MS_TRACE();

			// Delete retransmission buffer.
			delete this->retransmissionBuffer;
			this->retransmissionBuffer = nullptr;
		}

		flatbuffers::Offset<FBS::RtpStream::Stats> RtpStreamSend::FillBufferStats(
		  flatbuffers::FlatBufferBuilder& builder)
		{
			MS_TRACE();

			const int64_t nowMs = this->shared->GetTimeMs();

			const auto baseStats = RTP::RtpStream::FillBufferStats(builder);
			const auto stats     = FBS::RtpStream::CreateSendStats(
			  builder,
			  baseStats,
			  this->transmissionCounter.GetPacketCount(),
			  this->transmissionCounter.GetBytes(),
			  static_cast<uint64_t>(this->transmissionCounter.GetBitrate(nowMs).value_or(0)));

			return FBS::RtpStream::CreateStats(builder, FBS::RtpStream::StatsData::SendStats, stats.Union());
		}

		void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
		{
			MS_TRACE();

			RTP::RtpStream::SetRtx(payloadType, ssrc);

			this->rtxSeq = Utils::Crypto::GetRandomUInt<uint16_t>(0, 0xFFFF);
		}

		RtpStreamSend::ReceivePacketResult RtpStreamSend::ReceivePacket(
		  RTP::Packet* packet, const RTP::SharedPacket& sharedPacket)
		{
			MS_TRACE();

			MS_ASSERT(
			  packet->GetSsrc() == this->params.ssrc, "RTP packet SSRC does not match the encodings SSRC");

			// Call the parent method.
			if (!RtpStream::ReceiveStreamPacket(packet))
			{
				return ReceivePacketResult::DISCARDED;
			}

			UpdateUnsentSeqNumbers(packet->GetSequenceNumber());

			bool stored{ false };

			// If NACK is enabled, store the packet into the buffer.
			if (this->retransmissionBuffer)
			{
				// Check if the packet is already stored.
				if (this->retransmissionBuffer->Get(packet->GetSequenceNumber()))
				{
					// Packet already stored, do not resend it since the receiver will
					// fail decrypting it and this packet will be considered not received
					// in the RTCP transport feedback affecting the bandwidth estimation.

					MS_DEBUG_DEV(
					  "packet already stored in retransmission buffer [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
					  packet->GetSsrc(),
					  packet->GetSequenceNumber());

					return ReceivePacketResult::DISCARDED;
				}

				stored = this->retransmissionBuffer->Insert(packet, sharedPacket, this->shared->GetTimeMs());
			}

			// Increase transmission counter.
			this->transmissionCounter.Update(packet);

			return stored ? ReceivePacketResult::ACCEPTED_AND_STORED
			              : ReceivePacketResult::ACCEPTED_AND_NOT_STORED;
		}

		void RtpStreamSend::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
		{
			MS_TRACE();

			this->nackCount++;

			for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
			{
				const RTC::RTCP::FeedbackRtpNackItem* nackItem = *it;

				this->nackPacketCount += nackItem->CountRequestedPackets();

				if (!this->retransmissionBuffer)
				{
					continue;
				}

				// Queue the requested packets in the order they are requested: first the
				// one in the packet id and then those in the bitmask. A packet already
				// pending is not queued again since it will be retransmitted anyway.
				uint16_t seq     = nackItem->GetPacketId();
				uint16_t bitmask = nackItem->GetLostPacketBitmask();
				bool requested{ true };

				while (requested || bitmask != 0)
				{
					if (requested)
					{
						const bool inserted = this->pendingRetransmissionsSet.insert(seq).second;

						if (inserted)
						{
							this->pendingRetransmissionsQueue.push_back(seq);
						}
					}

					requested = (bitmask & 1) != 0;
					bitmask >>= 1;
					seq++;
				}
			}

			// If NACK is not supported, exit.
			if (!this->retransmissionBuffer)
			{
				MS_WARN_TAG(rtx, "NACK not supported");

				return;
			}

			// If a retransmission iteration is already scheduled, nothing else to do.
			if (this->retransmissionTimer->IsActive())
			{
				return;
			}

			// Otherwise retransmit the first pending packets right away and schedule
			// the next iteration if there are retransmissions left.
			RetransmitPendingPackets();

			if (!this->pendingRetransmissionsQueue.empty())
			{
				this->retransmissionTimer->Start(RetransmissionIntervalMs);
			}
		}

		void RtpStreamSend::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
		{
			MS_TRACE();

			switch (messageType)
			{
				case RTC::RTCP::FeedbackPs::MessageType::PLI:
				{
					this->pliCount++;

					break;
				}

				case RTC::RTCP::FeedbackPs::MessageType::FIR:
				{
					this->firCount++;

					break;
				}

				default:;
			}
		}

		std::optional<RtpStreamSend::Loss> RtpStreamSend::ReceiveRtcpReceiverReport(
		  RTC::RTCP::ReceiverReport* report, int64_t receivedAtUs)
		{
			MS_TRACE();

			/* Calculate RTT. */

			// Get the NTP representation of the time at which the Receiver Report
			// arrived, which is what the round trip is measured against.
			const auto ntp = Utils::Time::TimeUsToNtp(receivedAtUs + this->shared->GetNtpOffsetUs());

			// Get the compact NTP representation of the arrival time.
			uint32_t compactNtp = (ntp.seconds & 0x0000FFFF) << 16;

			compactNtp |= (ntp.fractions & 0xFFFF0000) >> 16;

			const uint32_t lastSr = report->GetLastSenderReport();
			const uint32_t dlsr   = report->GetDelaySinceLastSenderReport();

			// If no Sender Report was received by the remote endpoint yet, the Receiver
			// Report carries no RTT, so the last one is kept.
			//
			// NOTE: The subtraction wraps around along with the compact NTP
			// representation, which is what the conversion expects.
			if (lastSr != 0)
			{
				this->rttMs =
				  static_cast<float>(Utils::Time::CompactNtpRttToTimeUs(compactNtp - dlsr - lastSr)) / 1000;
			}

			this->jitter = static_cast<float>(report->GetJitter());

			// Work out how much of what it reports as lost really was lost on the way
			// to it, which is the only part this link is answerable for.
			const auto loss = UpdateSendLoss(report);

			// Update the score with the received RR.
			UpdateScore(loss);

			return loss;
		}

		std::optional<RtpStreamSend::Loss> RtpStreamSend::UpdateSendLoss(RTC::RTCP::ReceiverReport* report)
		{
			MS_TRACE();

			// Not a single packet has been sent, so there is no stretch of the stream
			// this report can be measuring.
			if (!this->lastRrSeq.has_value())
			{
				return std::nullopt;
			}

			// The remote endpoint counts its cycles from the first packet it saw, so
			// the extended value it reports is not in our numbering. Only the 16 bits
			// that travelled on the wire are, and this places them where they belong
			// without disturbing what the unwrapper knows.
			const int64_t rrSeq =
			  this->seqUnwrapper.PeekUnwrap(static_cast<uint16_t>(report->GetLastSeq())).GetValue();
			// NOTE: It is signed and may go down, since a duplicate counts as a packet
			// received twice.
			const int32_t totalLost = std::max<int32_t>(report->GetTotalLost(), 0);

			// A report that goes backwards is one that got reordered. Taking it would
			// move the mark back and count a stretch of the stream twice.
			if (rrSeq <= this->lastRrSeq.value())
			{
				return std::nullopt;
			}

			// What was never sent within the interval is what the remote endpoint
			// counts as lost without this link having lost it.
			const auto beginSeq       = this->unsentSeqs.upper_bound(this->lastRrSeq.value());
			const auto endSeq         = this->unsentSeqs.upper_bound(rrSeq);
			const auto unsentSeqCount = static_cast<int64_t>(std::distance(beginSeq, endSeq));

			// Everything up to what this report has seen is settled.
			this->unsentSeqs.erase(this->unsentSeqs.begin(), endSeq);

			// The numbering was reset and the remote endpoint went on counting over the
			// whole session, so there is nothing this report can be compared against. It
			// only leaves the marks for the next one.
			if (!this->lastRrTotalLost.has_value())
			{
				this->lastRrSeq       = rrSeq;
				this->lastRrTotalLost = totalLost;

				return std::nullopt;
			}

			const int64_t expectedPackets = (rrSeq - this->lastRrSeq.value()) - unsentSeqCount;
			// A hole already counted by a previous report and filled in since makes
			// the remote endpoint report fewer lost packets than before, so the
			// difference may well come out negative.
			const int64_t lostPackets =
			  int64_t{ totalLost } - this->lastRrTotalLost.value() - unsentSeqCount;
			// What is reported cannot go backwards, so it takes the count held between
			// nothing lost and everything expected lost.
			const int64_t reportedLostPackets = std::clamp<int64_t>(lostPackets, 0, expectedPackets);

			this->lastRrSeq       = rrSeq;
			this->lastRrTotalLost = totalLost;

			// NOTE: It is a count over the whole life of the stream, so it is held below
			// the maximum of its type rather than let overflow.
			this->packetsLost = static_cast<int32_t>(std::min<int64_t>(
			  int64_t{ this->packetsLost } + reportedLostPackets, std::numeric_limits<int32_t>::max()));
			// NOTE: The fraction is in 1/256 units, hence the shift, which is the
			// scale a Receiver Report uses for its own.
			this->fractionLost =
			  expectedPackets > 0
			    ? static_cast<uint8_t>(std::min<int64_t>((reportedLostPackets << 8) / expectedPackets, 255))
			    : 0;

			return Loss{ .lostPackets = lostPackets, .expectedPackets = expectedPackets };
		}

		void RtpStreamSend::ReceiveRtcpXrReceiverReferenceTime(
		  RTC::RTCP::ReceiverReferenceTime* report, int64_t receivedAtUs)
		{
			MS_TRACE();

			uint32_t compactNtp = report->GetNtpSec() << 16;

			compactNtp += report->GetNtpFrac() >> 16;

			this->lastReceiverReferenceTime = ReceiverReferenceTime{
				.compactNtp   = compactNtp,
				.receivedAtUs = receivedAtUs,
			};
		}

		RTC::RTCP::SenderReport* RtpStreamSend::GetRtcpSenderReport(int64_t nowUs)
		{
			MS_TRACE();

			if (this->transmissionCounter.GetPacketCount() == 0)
			{
				return nullptr;
			}

			// A stream that stopped sending cannot tell where its RTP timeline is now, and
			// extrapolating would claim RTP timestamps of packets never sent.
			if ((nowUs - this->maxPacketAtUs) / 1000 > RtpStreamSend::MaxSenderReportReferenceAgeMs)
			{
				return nullptr;
			}

			const auto ntp     = Utils::Time::TimeUsToNtp(nowUs + this->shared->GetNtpOffsetUs());
			auto* const report = new RTC::RTCP::SenderReport();

			// Calculate TS difference between now and the instant at which the media in the
			// packet holding the highest RTP timestamp was captured, falling back to the
			// instant that packet was seen while the capture instant cannot be told.
			const int64_t referenceUs = this->maxPacketCaptureAtUs.value_or(this->maxPacketAtUs);
			// NOTE: The capture instant is an estimation, so it may land ahead of now.
			const int64_t diffUs = nowUs > referenceUs ? nowUs - referenceUs : 0;
			const int64_t diffTs = (diffUs * GetClockRate()) / 1000000;
			const auto rtpTs     = static_cast<uint32_t>(this->maxPacketTs + diffTs);

			report->SetSsrc(GetSsrc());
			// NOTE: Both fields are 32 bits wide and are meant to wrap, so the counters
			// are truncated into them on purpose.
			report->SetPacketCount(static_cast<uint32_t>(this->transmissionCounter.GetPacketCount()));
			report->SetOctetCount(static_cast<uint32_t>(this->transmissionCounter.GetBytes()));
			report->SetNtpSec(ntp.seconds);
			report->SetNtpFrac(ntp.fractions);
			report->SetRtpTs(rtpTs);

			// Update info about last Sender Report.
			//
			// NOTE: It is the very instant announced in the report above, so that the
			// mapping means the same thing here and in a receive stream.
			this->lastSenderReportMapping = RTP::RtpStream::SenderReportMapping{
				.ntpUs = nowUs + this->shared->GetNtpOffsetUs(),
				.ts    = rtpTs,
			};

			return report;
		}

		RTC::RTCP::DelaySinceLastRr::SsrcInfo* RtpStreamSend::GetRtcpXrDelaySinceLastRrSsrcInfo(int64_t nowUs)
		{
			MS_TRACE();

			if (!this->lastReceiverReferenceTime.has_value())
			{
				return nullptr;
			}

			const auto& receiverReferenceTime = this->lastReceiverReferenceTime.value();
			// Get delay in microseconds.
			const int64_t delayUs = nowUs - receiverReferenceTime.receivedAtUs;
			// Express delay in units of 1/65536 seconds.
			auto dlrr = static_cast<uint32_t>((delayUs / 1000000) << 16);

			dlrr |= static_cast<uint32_t>(((delayUs % 1000000) * 65536) / 1000000);

			auto* const ssrcInfo = new RTC::RTCP::DelaySinceLastRr::SsrcInfo();

			ssrcInfo->SetSsrc(GetSsrc());
			ssrcInfo->SetDelaySinceLastReceiverReport(dlrr);
			ssrcInfo->SetLastReceiverReport(receiverReferenceTime.compactNtp);

			return ssrcInfo;
		}

		RTC::RTCP::SdesChunk* RtpStreamSend::GetRtcpSdesChunk()
		{
			MS_TRACE();

			const auto& cname     = GetCname();
			auto* const sdesChunk = new RTC::RTCP::SdesChunk(GetSsrc());
			auto* const sdesItem =
			  new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

			sdesChunk->AddItem(sdesItem);

			return sdesChunk;
		}

		void RtpStreamSend::Pause()
		{
			MS_TRACE();

			// Clear retransmission buffer.
			if (this->retransmissionBuffer)
			{
				this->retransmissionBuffer->Clear();
			}

			// Discard pending retransmissions.
			this->pendingRetransmissionsQueue.clear();
			this->pendingRetransmissionsSet.clear();

			if (this->retransmissionTimer)
			{
				this->retransmissionTimer->Stop();
			}

			// Reset jitter.
			this->jitter = 0;

			// Nothing is being measured while the stream is paused, and the numbering
			// goes on where it was once it resumes, so the marks the next Receiver
			// Report is measured against are left alone.
			this->fractionLost = 0;
		}

		void RtpStreamSend::Resume()
		{
			MS_TRACE();
		}

		void RtpStreamSend::UpdateUnsentSeqNumbers(uint16_t seq)
		{
			MS_TRACE();

			const int64_t unwrappedSeq = this->seqUnwrapper.Unwrap(seq).GetValue();

			// Nothing was sent before this one, so it leaves nothing behind. It is also
			// where the interval of the first Receiver Report starts, so that the first
			// one measures from here rather than from nowhere.
			if (!this->highestSentSeq.has_value())
			{
				this->highestSentSeq = unwrappedSeq;
				this->lastRrSeq      = unwrappedSeq - 1;

				return;
			}

			// It arrived late and fills in one that was taken for never sent.
			if (unwrappedSeq <= this->highestSentSeq.value())
			{
				this->unsentSeqs.erase(unwrappedSeq);

				return;
			}

			// Whatever it skipped was never handed to this stream and never will be:
			// it is what the uplink of the Producer lost and nobody could forward.
			//
			// NOTE: Only the most recent ones are taken, since anything older than that
			// is what the bound below would drop right away anyway, and a numbering
			// that jumps far ahead must not cost a walk over half the sequence number
			// space.
			const int64_t firstUnsentSeq = std::max<int64_t>(
			  this->highestSentSeq.value() + 1,
			  unwrappedSeq - static_cast<int64_t>(RtpStreamSend::MaxUnsentSeqNumbers));

			for (int64_t unsentSeq{ firstUnsentSeq }; unsentSeq < unwrappedSeq; ++unsentSeq)
			{
				this->unsentSeqs.insert(unsentSeq);
			}

			this->highestSentSeq = unwrappedSeq;

			// Only a Receiver Report drains this, so a remote endpoint that stops
			// reporting must not make it grow without end. The oldest ones go, which
			// are the ones the next report is least likely to name.
			while (this->unsentSeqs.size() > RtpStreamSend::MaxUnsentSeqNumbers)
			{
				this->unsentSeqs.erase(this->unsentSeqs.begin());
			}
		}

		int64_t RtpStreamSend::GetBitrate(
		  int64_t /*nowMs*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
		{
			MS_TRACE();

			MS_ABORT("invalid method call");
		}

		int64_t RtpStreamSend::GetSpatialLayerBitrate(int64_t /*nowMs*/, uint8_t /*spatialLayer*/)
		{
			MS_TRACE();

			MS_ABORT("invalid method call");
		}

		int64_t RtpStreamSend::GetLayerBitrate(
		  int64_t /*nowMs*/, uint8_t /*spatialLayer*/, uint8_t /*temporalLayer*/)
		{
			MS_TRACE();

			MS_ABORT("invalid method call");
		}

		// This method takes pending retransmissions from the front of the queue and
		// retransmits their packets until MaxRetransmittedPacketsPerIteration packets
		// have been retransmitted or the queue is empty. Pending retransmissions whose
		// packet is not retransmitted don't count.
		//
		// If RTX is used the stored packet will be RTX encoded now.
		void RtpStreamSend::RetransmitPendingPackets()
		{
			MS_TRACE();

			const int64_t nowMs = this->shared->GetTimeMs();
			const int64_t rttMs = (this->rttMs > 0.0f ? static_cast<int64_t>(this->rttMs) : DefaultRttMs);

			size_t numRetransmittedPackets{ 0 };

			while (!this->pendingRetransmissionsQueue.empty() &&
			       numRetransmittedPackets < MaxRetransmittedPacketsPerIteration)
			{
				const uint16_t seq = this->pendingRetransmissionsQueue.front();

				this->pendingRetransmissionsQueue.pop_front();
				this->pendingRetransmissionsSet.erase(seq);

				RTP::RetransmissionBuffer::Item* const item = this->retransmissionBuffer->Get(seq);

				// Packet not found.
				if (!item)
				{
					MS_DEBUG_DEV("ignoring retransmission for a packet not stored [seq:%" PRIu16 "]", seq);

					continue;
				}
				// Don't resend the packet if it was stored too long ago.
				else if (this->retransmissionBuffer->IsTooOld(item, nowMs))
				{
					MS_DEBUG_DEV(
					  "ignoring retransmission for a packet stored too long ago [seq:%" PRIu16
					  ", stored:%" PRIi64 " ms ago]",
					  seq,
					  nowMs - item->storedAtMs);

					continue;
				}
				// Don't resend the packet if it was resent in the last RTT ms.
				else if (item->resentAtMs != 0 && nowMs - item->resentAtMs <= rttMs)
				{
					MS_DEBUG_TAG(
					  rtx,
					  "ignoring retransmission for a packet already resent in the last RTT ms "
					  "[seq:%" PRIu16 ", rtt:%" PRIi64 " ms]",
					  item->sequenceNumber,
					  rttMs);

					continue;
				}

				// Save when this packet was resent.
				item->resentAtMs = nowMs;

				// Increase the number of times this packet was sent.
				item->sentTimes++;

				MS_ASSERT(
				  item->sharedPacket.HasPacket(),
				  "stored item doesn't contain a packet [ssrc:%" PRIu32 ", seq:%" PRIu16
				  ", timestamp:%" PRIu32 "]",
				  item->ssrc,
				  item->sequenceNumber,
				  item->timestamp);

				auto* const packet = item->sharedPacket.GetPacket();

				// Keep the values of the original packet received by the Consumer.
				const auto origSsrc      = packet->GetSsrc();
				const auto origSeq       = packet->GetSequenceNumber();
				const auto origTimestamp = packet->GetTimestamp();
				const auto origMarker    = packet->HasMarker();

				std::string origMid;

				// Put correct info into the packet.
				packet->SetSsrc(item->ssrc);
				packet->SetSequenceNumber(item->sequenceNumber);
				packet->SetTimestamp(item->timestamp);
				packet->SetMarker(item->marker);

				if (item->encoder != nullptr)
				{
					packet->EncodePayload(item->encoder.get());
				}

				// Update MID RTP extension value.
				if (!this->mid.empty())
				{
					packet->ReadMid(origMid);
					packet->UpdateMid(this->mid);
				}

				// If we use RTX, encode it.
				if (HasRtx())
				{
					// Increment RTX seq.
					this->rtxSeq++;

					packet->RtxEncode(this->params.rtxPayloadType, this->params.rtxSsrc, this->rtxSeq);
				}

				// Retransmit the packet.
				static_cast<RTP::RtpStreamSend::Listener*>(this->listener)
				  ->OnRtpStreamRetransmitRtpPacket(this, packet);

				// Mark the packet as retransmitted.
				RTP::RtpStream::PacketRetransmitted(packet);

				// Mark the packet as repaired (only if this is the first retransmission).
				if (item->sentTimes == 1)
				{
					RTP::RtpStream::PacketRepaired(packet);
				}

				// If we use RTX, restore it.
				if (HasRtx())
				{
					// Restore the packet.
					packet->RtxDecode(RtpStream::GetPayloadType(), item->ssrc);
				}

				// Restore MID.
				if (!this->mid.empty())
				{
					packet->UpdateMid(origMid);
				}

				// Restore payload.
				if (item->encoder != nullptr)
				{
					packet->RestorePayload();
				}

				// Restore RTP header fields.
				packet->SetSsrc(origSsrc);
				packet->SetSequenceNumber(origSeq);
				packet->SetTimestamp(origTimestamp);
				packet->SetMarker(origMarker);

				numRetransmittedPackets++;
			}
		}

		void RtpStreamSend::UpdateScore(const std::optional<Loss>& loss)
		{
			MS_TRACE();

			// Calculate the packets of this interval, which is what each counter has
			// grown by since the previous score update.
			const auto totalSentPackets          = this->transmissionCounter.GetPacketCount();
			const auto totalRepairedPackets      = this->packetsRepaired;
			const auto totalRetransmittedPackets = this->packetsRetransmitted;
			const auto sentPackets               = totalSentPackets - this->sentPriorScore;
			const auto retransmittedPackets = totalRetransmittedPackets - this->retransmittedPriorScore;

			auto repairedPackets = totalRepairedPackets - this->repairedPriorScore;

			this->sentPriorScore          = totalSentPackets;
			this->repairedPriorScore      = totalRepairedPackets;
			this->retransmittedPriorScore = totalRetransmittedPackets;

			// The Receiver Report measured no stretch of the stream, so there is nothing
			// to say about this interval. The counters above have been taken anyway, so
			// that the next one is measured over what it really covers.
			if (!loss.has_value())
			{
				return;
			}

			// We didn't send any packet.
			if (sentPackets == 0)
			{
				RTP::RtpStream::UpdateScore(10);

				return;
			}

			// Number of packets lost in this interval, which is what this link really
			// lost and not what the remote endpoint reported, since that one also
			// counts the sequence numbers that were never sent.
			//
			// NOTE: A report that gives back what a previous one was overcharged leaves
			// this interval with nothing lost.
			auto lostPackets = static_cast<uint64_t>(std::max<int64_t>(loss.value().lostPackets, 0));

			lostPackets     = std::min(lostPackets, sentPackets);
			repairedPackets = std::min(repairedPackets, lostPackets);

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[totalSentPackets:%" PRIu64 ", totalLostPackets:%" PRIi32 ", totalRepairedPackets:%" PRIu64,
			  totalSentPackets,
			  this->packetsLost,
			  totalRepairedPackets);

			MS_DEBUG_TAG(
			  score,
			  "fixed values [sentPackets:%" PRIu64 ", lostPackets:%" PRIu64 ", repairedPackets:%" PRIu64
			  ", retransmittedPackets:%" PRIu64,
			  sentPackets,
			  lostPackets,
			  repairedPackets,
			  retransmittedPackets);
#endif

			MS_ASSERT(
			  retransmittedPackets >= repairedPackets,
			  "repaired packets cannot be more than retransmitted ones");

			// With RTX the retransmissions go in a stream of their own, so the remote
			// endpoint keeps counting the packets they repair as lost and the repair
			// has to be discounted here. Without RTX a retransmission is the very packet
			// it repairs, so the remote endpoint counts it as received and has already
			// left it out of what it reports as lost.
			if (HasRtx())
			{
				const auto repairedRatio =
				  static_cast<float>(repairedPackets) / static_cast<float>(sentPackets);

				auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

				if (retransmittedPackets > 0)
				{
					repairedWeight *= static_cast<float>(repairedPackets) / retransmittedPackets;
				}

				lostPackets = static_cast<uint64_t>(lostPackets - (repairedPackets * repairedWeight));

#if MS_LOG_DEV_LEVEL == 3
				MS_DEBUG_TAG(
				  score,
				  "[repairedRatio:%f, repairedWeight:%f, new lostPackets:%" PRIu64 "]",
				  repairedRatio,
				  repairedWeight,
				  lostPackets);
#endif
			}

			const auto deliveredRatio =
			  static_cast<float>(sentPackets - lostPackets) / static_cast<float>(sentPackets);
			const auto score = static_cast<uint8_t>(std::round(std::pow(deliveredRatio, 4) * 10));

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[deliveredRatio:%f, lostPackets:%" PRIu64 ", score:%" PRIu8 "]",
			  deliveredRatio,
			  lostPackets,
			  score);
#endif

			// Call the parent method for update score.
			RTP::RtpStream::UpdateScore(score);
		}

		void RtpStreamSend::UserOnSequenceNumberReset()
		{
			MS_TRACE();

			// Clear retransmission buffer.
			if (this->retransmissionBuffer)
			{
				this->retransmissionBuffer->Clear();
			}

			// Discard pending retransmissions, since their sequence numbers belong to
			// the previous numbering.
			this->pendingRetransmissionsQueue.clear();
			this->pendingRetransmissionsSet.clear();

			if (this->retransmissionTimer)
			{
				this->retransmissionTimer->Stop();
			}

			ResetSendLoss();
		}

		void RtpStreamSend::ResetSendLoss()
		{
			MS_TRACE();

			// What was never sent belongs to a numbering that is not in use anymore,
			// and so does the mark the next Receiver Report would be measured against.
			this->seqUnwrapper.Reset();
			this->unsentSeqs.clear();
			this->highestSentSeq.reset();
			this->lastRrSeq.reset();
			// The remote endpoint counts what it has lost over the whole session and
			// does not reset along with us, so the next Receiver Report cannot be a
			// difference against anything and can only become the new mark.
			this->lastRrTotalLost.reset();
			// Nothing has been measured over this numbering, and holding on to what
			// was measured over the previous one would keep reporting it for as long
			// as the stream stays quiet.
			//
			// NOTE: The total of lost packets is not cleared, being a count over the
			// whole life of the stream rather than a measure of its current state.
			this->fractionLost = 0;
		}

		void RtpStreamSend::OnTimer(TimerHandleInterface* timer)
		{
			MS_TRACE();

			if (timer == this->retransmissionTimer.get())
			{
				RetransmitPendingPackets();

				// Schedule the next iteration if there are retransmissions left.
				if (!this->pendingRetransmissionsQueue.empty())
				{
					this->retransmissionTimer->Start(RetransmissionIntervalMs);
				}
			}
		}
	} // namespace RTP
} // namespace RTC
