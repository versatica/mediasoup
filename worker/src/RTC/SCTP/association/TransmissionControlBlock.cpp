#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include <cmath> // std::min()
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		alignas(4) thread_local static uint8_t PacketFactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

		/* Instance methods. */

		TransmissionControlBlock::TransmissionControlBlock(
		  AssociationListener& associationListener,
		  const SctpOptions& sctpOptions,
		  SharedInterface* shared,
		  PacketSender& packetSender,
		  // TODO: SCTP: Implement it.
		  // SendQueue& sendQueue,
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities,
		  std::function<bool()> isAssociationEstablished)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    shared(shared),
		    // TODO: SCTP: Implement it.
		    // sendQueue(sendQueue),
		    packetSender(packetSender),
		    localVerificationTag(localVerificationTag),
		    remoteVerificationTag(remoteVerificationTag),
		    localInitialTsn(localInitialTsn),
		    remoteInitialTsn(remoteInitialTsn),
		    remoteAdvertisedReceiverWindowCredit(remoteAdvertisedReceiverWindowCredit),
		    tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities),
		    isAssociationEstablished(std::move(isAssociationEstablished)),
		    t3RtxTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		        .maxRestarts         = std::nullopt })),
		    delayedAckTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .baseTimeoutMs       = sctpOptions.delayedAckMaxTimeoutMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = std::nullopt,
		        .maxRestarts         = 0 })),
		    rto(sctpOptions),
		    txErrorCounter(sctpOptions),
		    // TODO: SCTP: Implement.
		    // dataTracker(),
		    // TODO: SCTP: Implement.
		    // reassemblyQueue(),
		    retransmissionQueue(
		      this,
		      this->associationListener,
		      localInitialTsn,
		      remoteAdvertisedReceiverWindowCredit,
		      // TODO: SCTP: Implement
		      // this->sendQueue,
		      this->t3RtxTimer.get(),
		      sctpOptions,
		      negotiatedCapabilities.partialReliability,
		      negotiatedCapabilities.messageInterleaving),
		    streamResetHandler(
		      this->associationListener,
		      this->shared,
		      this,
		      // TODO: SCTP: Implement.
		      // std::addressof(this->dataTracker),
		      // std::addressof(this->reassemblyQueue),
		      std::addressof(this->retransmissionQueue)),
		    heartbeatHandler(this->associationListener, sctpOptions, this->shared, this)
		{
			MS_TRACE();
		}

		TransmissionControlBlock::~TransmissionControlBlock()
		{
			MS_TRACE();
		}

		void TransmissionControlBlock::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::TransmissionControlBlock>");

			MS_DUMP_CLEAN(indentation, "  local verification tag: %" PRIu32, this->localVerificationTag);
			MS_DUMP_CLEAN(indentation, "  remote verification tag: %" PRIu32, this->remoteVerificationTag);
			MS_DUMP_CLEAN(indentation, "  local initial tsn: %" PRIu32, this->localInitialTsn);
			MS_DUMP_CLEAN(indentation, "  remote initial tsn: %" PRIu32, this->remoteInitialTsn);
			MS_DUMP_CLEAN(
			  indentation,
			  "  remote advertised receiver window credit: %" PRIu32,
			  this->remoteAdvertisedReceiverWindowCredit);
			MS_DUMP_CLEAN(indentation, "  tie-tag: %" PRIu64, this->tieTag);

			this->negotiatedCapabilities.Dump(indentation + 1);

			this->rto.Dump(indentation + 1);

			this->txErrorCounter.Dump(indentation + 1);

			MS_DUMP_CLEAN(indentation, "</SCTP::TransmissionControlBlock>");
		}

		void TransmissionControlBlock::ObserveRttMs(uint64_t rttMs)
		{
			MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
			const auto prevRtoMs = this->rto.GetRtoMs();
#endif

			this->rto.ObserveRttMs(rttMs);

			MS_DEBUG_DEV(
			  "new rtt:%" PRIu64 ", previous rto:%" PRIu64 ", new rto:%" PRIu64 ", srtt:%" PRIu64,
			  rttMs,
			  prevRtoMs,
			  this->rto.GetRtoMs(),
			  this->rto.GetSrttMs());

			this->t3RtxTimer->SetBaseTimeoutMs(this->rto.GetRtoMs());

			const uint64_t delayedAckTimeoutMs = std::min(
			  static_cast<uint64_t>(this->rto.GetRtoMs() * 0.5), this->sctpOptions.delayedAckMaxTimeoutMs);

			this->delayedAckTimer->SetBaseTimeoutMs(delayedAckTimeoutMs);
		}

		std::unique_ptr<Packet> TransmissionControlBlock::CreatePacket() const
		{
			MS_TRACE();

			return CreatePacketWithVerificationTag(this->remoteVerificationTag);
		}

		std::unique_ptr<Packet> TransmissionControlBlock::CreatePacketWithVerificationTag(
		  uint32_t verificationTag) const
		{
			MS_TRACE();

			auto packet =
			  std::unique_ptr<Packet>(Packet::Factory(PacketFactoryBuffer, sizeof(PacketFactoryBuffer)));

			packet->SetSourcePort(this->sctpOptions.sourcePort);
			packet->SetDestinationPort(this->sctpOptions.destinationPort);
			packet->SetVerificationTag(verificationTag);

			return packet;
		}

		void TransmissionControlBlock::Send(Packet* packet)
		{
			MS_TRACE();

			this->packetSender.SendPacket(
			  packet,
			  /*writeChecksum*/ !this->negotiatedCapabilities.zeroChecksum);
		}

		void TransmissionControlBlock::SetRemoteStateCookie(std::vector<uint8_t> remoteStateCookie)
		{
			MS_TRACE();

			this->remoteStateCookie = std::move(remoteStateCookie);
		}

		void TransmissionControlBlock::ClearRemoteStateCookie()
		{
			MS_TRACE();

			this->remoteStateCookie.reset();
		}

		void TransmissionControlBlock::MaySendSackChunk()
		{
			MS_TRACE();

			// TODO: SCTP: Implement it.
			// if (!this->dataTracker.ShouldSendAckChunk(/*alsoIfDelayed*/ false))
			// {
			// 	return;
			// }

			// auto packet = CreatePacket();

			// TODO: SCTP: Here we must create a SackChunk in the Packet, however the
			// SackChunk is in theory generated by this->dataTracker... Let's see.
			// builder.Add(this->dataTracker.CreateSelectiveAck(this->reassemblyQueue.GetRemainingBytes()));

			// Send(packet.get());
		}

		void TransmissionControlBlock::MaybeSendForwardTsnChunk(Packet* packet, uint64_t nowMs)
		{
			MS_TRACE();

			if (nowMs >= this->limitForwardTsnUntilMs && this->retransmissionQueue.ShouldSendForwardTsn(nowMs))
			{
				if (this->negotiatedCapabilities.messageInterleaving)
				{
					this->retransmissionQueue.CreateIForwardTsn(packet);
				}
				else
				{
					this->retransmissionQueue.CreateForwardTsn(packet);
				}

				// https://datatracker.ietf.org/doc/html/rfc3758
				//
				// "IMPLEMENTATION NOTE: An implementation may wish to limit the number
				// of duplicate FORWARD TSN chunks it sends by ... waiting a full RTT
				// before sending a duplicate FORWARD TSN."
				// "Any delay applied to the sending of FORWARD TSN chunk SHOULD NOT
				// exceed 200ms and MUST NOT exceed 500ms".
				this->limitForwardTsnUntilMs = nowMs + std::min(uint64_t{ 200 }, this->rto.GetSrttMs());
			}
		}

		void TransmissionControlBlock::MaySendFastRetransmit()
		{
			MS_TRACE();

			if (!this->retransmissionQueue.HasDataToBeFastRetransmitted())
			{
				return;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-7.2.4
			//
			// "Determine how many of the earliest (i.e., lowest TSN) DATA chunks
			// marked for retransmission will fit into a single packet, subject to
			// constraint of the path MTU of the destination transport address to
			// which the packet is being sent. Call this value K. Retransmit those
			// K DATA chunks in a single packet.  When a Fast Retransmit is being
			// performed, the sender SHOULD ignore the value of cwnd and SHOULD NOT
			// delay retransmission for this single packet."

			auto packet = CreatePacket();
			const auto result =
			  this->retransmissionQueue.GetChunksForFastRetransmit(packet->GetAvailableLength());

			for (const auto& [tsn, data] : result)
			{
				if (this->negotiatedCapabilities.messageInterleaving)
				{
					auto* iDataChunk = packet->BuildChunkInPlace<IDataChunk>();

					iDataChunk->SetTsn(tsn);
					// TODO: SCTP: Implement.
					// iDataChunk->SetUserData(data);
					iDataChunk->Consolidate();
				}
				else
				{
					auto* dataChunk = packet->BuildChunkInPlace<DataChunk>();

					dataChunk->SetTsn(tsn);
					// TODO: SCTP: Implement.
					// dataChunk->SetUserData(data);
					dataChunk->Consolidate();
				}
			}

			Send(packet.get());
		}

		void TransmissionControlBlock::OnT3RtxTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->t3RtxTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "T3-rtx timer has expired [%zu/%s]",
			  this->t3RtxTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// In the COOKIE_ECHO state, let the T1-COOKIE timer trigger
			// retransmissions, to avoid having two timers doing that.
			if (this->remoteStateCookie.has_value())
			{
				MS_DEBUG_DEV("not retransmitting as T1-cookie is active");
			}
			else
			{
				if (IncrementTxErrorCounter("t3-rtx expired"))
				{
					this->retransmissionQueue.HandleT3RtxTimerExpiry();

					// const uint64_t nowMs = DepLibUV::GetTimeMs();

					// TODO: SCTP: Implement
					// SendBufferedPackets(nowMs);
				}
			}
		}

		void TransmissionControlBlock::OnDelayedAckTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->delayedAckTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "delayer ack timer has expired [%zu/%s]",
			  this->delayedAckTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO: SCTP: Implement it.
			// this->dataTracker.HandleDelayedAckTimerExpiry();

			MaySendSackChunk();
		}

		void TransmissionControlBlock::OnTimer(
		  BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->t3RtxTimer.get())
			{
				OnT3RtxTimer(baseTimeoutMs, stop);
			}
			else if (backoffTimer == this->delayedAckTimer.get())
			{
				OnDelayedAckTimer(baseTimeoutMs, stop);
			}
		}

		void TransmissionControlBlock::OnRetransmissionQueueNewRttMs(uint64_t newRttMs)
		{
			MS_TRACE();

			ObserveRttMs(newRttMs);
		}

		void TransmissionControlBlock::OnRetransmissionQueueClearRetransmissionCounter()
		{
			MS_TRACE();

			this->txErrorCounter.Clear();
		}
	} // namespace SCTP
} // namespace RTC
