#define MS_CLASS "RTC::RTP::RtpStreamSend"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/RtpStreamSend.hpp"
#include "Logger.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include <cmath> // std::pow(), std::round()

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

			auto baseStats = RTP::RtpStream::FillBufferStats(builder);
			auto stats     = FBS::RtpStream::CreateSendStats(
			  builder,
			  baseStats,
			  this->transmissionCounter.GetPacketCount(),
			  this->transmissionCounter.GetBytes(),
			  static_cast<uint64_t>(this->transmissionCounter.GetBitrate(nowMs).value_or(0)),
			  this->sendLossState->GetFractionLost());

			return FBS::RtpStream::CreateStats(builder, FBS::RtpStream::StatsData::SendStats, stats.Union());
		}

		void RtpStreamSend::SetRtx(uint8_t payloadType, uint32_t ssrc)
		{
			MS_TRACE();

			RTP::RtpStream::SetRtx(payloadType, ssrc);

			this->rtxSeq = Utils::Crypto::GetRandomUInt<uint16_t>(0u, 0xFFFF);
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

		void RtpStreamSend::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report, int64_t receivedAtUs)
		{
			MS_TRACE();

			/* Calculate RTT. */

			// Get the NTP representation of the time at which the Receiver Report
			// arrived, which is what the round trip is measured against.
			auto ntp = Utils::Time::TimeUsToNtp(receivedAtUs + this->shared->GetNtpOffsetUs());

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

			this->packetsLost  = report->GetTotalLost();
			this->fractionLost = report->GetFractionLost();
			this->jitter       = static_cast<float>(report->GetJitter());

			// Update the score with the received RR.
			UpdateScore(report);

			// Update the send fraction loss with the received RR.
			this->sendLossState->Update(report);
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

			if (this->transmissionCounter.GetPacketCount() == 0u)
			{
				return nullptr;
			}

			// A stream that stopped sending cannot tell where its RTP timeline is now, and
			// extrapolating would claim RTP timestamps of packets never sent.
			if ((nowUs - this->maxPacketAtUs) / 1000 > RtpStreamSend::MaxSenderReportReferenceAgeMs)
			{
				return nullptr;
			}

			auto ntp     = Utils::Time::TimeUsToNtp(nowUs + this->shared->GetNtpOffsetUs());
			auto* report = new RTC::RTCP::SenderReport();

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

			auto* ssrcInfo = new RTC::RTCP::DelaySinceLastRr::SsrcInfo();

			ssrcInfo->SetSsrc(GetSsrc());
			ssrcInfo->SetDelaySinceLastReceiverReport(dlrr);
			ssrcInfo->SetLastReceiverReport(receiverReferenceTime.compactNtp);

			return ssrcInfo;
		}

		RTC::RTCP::SdesChunk* RtpStreamSend::GetRtcpSdesChunk()
		{
			MS_TRACE();

			const auto& cname = GetCname();
			auto* sdesChunk   = new RTC::RTCP::SdesChunk(GetSsrc());
			auto* sdesItem =
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

			// Reset the send loss state.
			this->ResetSendLossState();

			// Reset jitter.
			this->jitter = 0;
		}

		void RtpStreamSend::Resume()
		{
			MS_TRACE();
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
				  ->OnRtpStreamRetransmitRtpPacket(this, packet, item->sequenceNumber);

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

		void RtpStreamSend::UpdateScore(RTC::RTCP::ReceiverReport* report)
		{
			MS_TRACE();

			// Calculate number of packets sent in this interval.
			const auto totalSent = this->transmissionCounter.GetPacketCount();
			const auto sent      = totalSent - this->sentPriorScore;

			this->sentPriorScore = totalSent;

			// Calculate number of packets lost in this interval.
			const int32_t totalLost = report->GetTotalLost() > 0 ? report->GetTotalLost() : 0;

			uint64_t lost;

			if (totalLost < this->lostPriorScore)
			{
				lost = 0;
			}
			else
			{
				lost = totalLost - this->lostPriorScore;
			}

			this->lostPriorScore = totalLost;

			// Calculate number of packets repaired in this interval.
			const auto totalRepaired = this->packetsRepaired;

			auto repaired = totalRepaired - this->repairedPriorScore;

			this->repairedPriorScore = totalRepaired;

			// Calculate number of packets retransmitted in this interval.
			const auto totatRetransmitted = this->packetsRetransmitted;
			const auto retransmitted      = totatRetransmitted - this->retransmittedPriorScore;

			this->retransmittedPriorScore = totatRetransmitted;

			// We didn't send any packet.
			if (sent == 0)
			{
				RTP::RtpStream::UpdateScore(10);

				return;
			}

			lost     = std::min(lost, sent);
			repaired = std::min(repaired, lost);

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[totalSent:%" PRIu64 ", totalLost:%" PRIi32 ", totalRepaired:%" PRIu64,
			  totalSent,
			  totalLost,
			  totalRepaired);

			MS_DEBUG_TAG(
			  score,
			  "fixed values [sent:%" PRIu64 ", lost:%" PRIu64 ", repaired:%" PRIu64
			  ", retransmitted:%" PRIu64,
			  sent,
			  lost,
			  repaired,
			  retransmitted);
#endif

			auto repairedRatio  = static_cast<float>(repaired) / static_cast<float>(sent);
			auto repairedWeight = std::pow(1 / (repairedRatio + 1), 4);

			MS_ASSERT(retransmitted >= repaired, "repaired packets cannot be more than retransmitted ones");

			if (retransmitted > 0)
			{
				repairedWeight *= static_cast<float>(repaired) / retransmitted;
			}

			lost = static_cast<uint64_t>(lost - (repaired * repairedWeight));

			auto deliveredRatio = static_cast<float>(sent - lost) / static_cast<float>(sent);
			auto score          = static_cast<uint8_t>(std::round(std::pow(deliveredRatio, 4) * 10));

#if MS_LOG_DEV_LEVEL == 3
			MS_DEBUG_TAG(
			  score,
			  "[deliveredRatio:%f, repairedRatio:%f, repairedWeight:%f, new lost:%" PRIu64
			  ", score:%" PRIu8 "]",
			  deliveredRatio,
			  repairedRatio,
			  repairedWeight,
			  lost,
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

			// Reset the send loss state.
			this->ResetSendLossState();
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

		void RtpStreamSend::ResetSendLossState()
		{
			MS_TRACE();

			this->sendLossState->Reset();
		}

		/* SendLossState */

		void RtpStreamSend::SendLossState::Reset()
		{
			MS_TRACE();

			this->initialized = false;
			this->rrAnchor.reset();
			this->epoch++;
		}

		void RtpStreamSend::SendLossState::RegisterSent(uint32_t epoch, uint32_t extSeq)
		{
			MS_TRACE();

			// The packet was sent in a previous epoch.
			if (epoch != this->epoch)
			{
				return;
			}

			if (!this->initialized)
			{
				this->initialized   = true;
				this->lowestExtSeq  = extSeq - (static_cast<uint32_t>(BitmapSize) / 2);
				this->highestExtSeq = extSeq;
				this->bitmap.reset();
				this->bitmap.set(static_cast<size_t>(extSeq) % BitmapSize);

				return;
			}

			// Older than the interval being tracked.
			if (static_cast<int32_t>(extSeq - this->lowestExtSeq) < 0)
			{
				return;
			}

			// Slide the window right so that the sequence number fits in it.
			if (static_cast<size_t>(extSeq - this->lowestExtSeq) >= BitmapSize)
			{
				const uint32_t newLowestExtSeq = extSeq - static_cast<uint32_t>(BitmapSize) + 1;
				const size_t slide             = newLowestExtSeq - this->lowestExtSeq;

				if (slide >= BitmapSize)
				{
					this->bitmap.reset();
				}
				else
				{
					// Clear the slots of the sequence numbers that leave the window, since they
					// are taken over by the ones that enter it.
					for (uint32_t seq = this->lowestExtSeq; seq != newLowestExtSeq; ++seq)
					{
						this->bitmap.reset(static_cast<size_t>(seq) % BitmapSize);
					}
				}

				this->lowestExtSeq = newLowestExtSeq;
			}

			this->bitmap.set(static_cast<size_t>(extSeq) % BitmapSize);

			if (static_cast<int32_t>(extSeq - this->highestExtSeq) > 0)
			{
				this->highestExtSeq = extSeq;
			}
		}

		std::optional<uint32_t> RtpStreamSend::SendLossState::MapReportedExtSeq(uint32_t rrHighestExtSeq) const
		{
			MS_TRACE();

			uint32_t extHighest =
			  (this->highestExtSeq & 0xFFFF0000u) | static_cast<uint16_t>(rrHighestExtSeq);

			// Roll back to the previous cycle if rrHighestExtSeq is ahead of the highest sequence
			// number sent.
			if (static_cast<int32_t>(extHighest - this->highestExtSeq) > 0)
			{
				extHighest -= 0x10000;
			}

			if (static_cast<int32_t>(extHighest - this->lowestExtSeq) < 0)
			{
				return std::nullopt;
			}

			return extHighest;
		}

		void RtpStreamSend::SendLossState::Update(RTC::RTCP::ReceiverReport* report)
		{
			MS_TRACE();

			const uint32_t rrHighestExtSeq = report->GetLastSeq();
			const int32_t totalLost        = report->GetTotalLost();

			if (!this->initialized)
			{
				return;
			}

			const auto extHighest = this->MapReportedExtSeq(rrHighestExtSeq);

			if (!extHighest.has_value())
			{
				MS_WARN_TAG(
				  rtp,
				  "Receiver Report does not resolve to a sent sequence number, ignoring it "
				  "[rrHighestExtSeq:%" PRIu32 ", lowestExtSeq:%" PRIu32 ", highestExtSeq:%" PRIu32 "]",
				  rrHighestExtSeq,
				  this->lowestExtSeq,
				  this->highestExtSeq);

				return;
			}

			const auto offset = static_cast<int32_t>(*extHighest - this->lowestExtSeq);

			if (offset < 0 || std::cmp_greater_equal(offset, BitmapSize))
			{
				MS_WARN_TAG(
				  rtp,
				  "Receiver Report extHighestSeq out of the window, ignoring it "
				  "[rrHighestExtSeq:%" PRIu32 ", lowestExtSeq:%" PRIu32 ", highestExtSeq:%" PRIu32 "]",
				  rrHighestExtSeq,
				  this->lowestExtSeq,
				  this->highestExtSeq);

				return;
			}

			if (!this->rrAnchor.has_value())
			{
				this->rrAnchor = RrAnchor{ .extSeq = *extHighest, .totalLost = totalLost };

				return;
			}

			const auto priorRRAnchor = this->rrAnchor;
			this->rrAnchor           = RrAnchor{ .extSeq = *extHighest, .totalLost = totalLost };

			if (static_cast<int32_t>(*extHighest - priorRRAnchor->extSeq) < 0)
			{
				MS_WARN_TAG(
				  rtp,
				  "Receiver Report lastSeq rollback, ignoring it "
				  "[rrHighestExtSeq:%" PRIu32 ", extHighestSeq:%" PRIu32 ", anchorExtSeq:%" PRIu32
				  ", highestExtSeq:%" PRIu32 "]",
				  rrHighestExtSeq,
				  *extHighest,
				  priorRRAnchor->extSeq,
				  this->highestExtSeq);

				return;
			}

			const uint32_t intervalExpected = *extHighest - priorRRAnchor->extSeq;

			if (
			  static_cast<int32_t>(priorRRAnchor->extSeq - this->lowestExtSeq) < 0 ||
			  std::cmp_greater_equal(intervalExpected, BitmapSize))
			{
				MS_WARN_TAG(
				  rtp,
				  "Receiver Report seq interval out of the window, ignoring it "
				  "[rrHighestExtSeq:%" PRIu32 ", extHighestSeq:%" PRIu32 ", anchorExtSeq:%" PRIu32
				  ", lowestExtSeq:%" PRIu32 ", intervalExpected:%" PRIu32 "]",
				  rrHighestExtSeq,
				  *extHighest,
				  priorRRAnchor->extSeq,
				  this->lowestExtSeq,
				  intervalExpected);

				return;
			}

			// No new packet in the interval.
			if (intervalExpected == 0)
			{
				return;
			}

			uint32_t sent = 0;

			for (uint32_t seq = priorRRAnchor->extSeq + 1; seq != *extHighest + 1; ++seq)
			{
				const size_t idx = static_cast<size_t>(seq) % BitmapSize;

				if (this->bitmap.test(idx))
				{
					sent++;
					this->bitmap.reset(idx);
				}
			}

			// The interval was accounted, but nothing of it was sent, so every loss the endpoint
			// reported in it belongs to a hole.
			if (sent == 0)
			{
				this->fractionLost = 0;

				return;
			}

			const uint32_t unsent  = intervalExpected - sent;
			const int32_t sendLost = totalLost - priorRRAnchor->totalLost - static_cast<int32_t>(unsent);
			const int32_t lost     = std::clamp(sendLost, 0, static_cast<int32_t>(sent));
			this->fractionLost     = static_cast<uint8_t>(255 * lost / sent);
		}

	} // namespace RTP
} // namespace RTC
