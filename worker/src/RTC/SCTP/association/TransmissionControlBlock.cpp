#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		alignas(4) static thread_local uint8_t PacketFactoryBuffer[65536];

		/* Instance methods. */

		TransmissionControlBlock::TransmissionControlBlock(
		  AssociationListenerInterface& associationListener,
		  const SctpOptions& sctpOptions,
		  SharedInterface* shared,
		  SendQueueInterface& sendQueue,
		  PacketSender& packetSender,
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities,
		  size_t maxPacketLength,
		  std::function<bool()> isAssociationEstablished)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    shared(shared),
		    packetSender(packetSender),
		    localVerificationTag(localVerificationTag),
		    remoteVerificationTag(remoteVerificationTag),
		    localInitialTsn(localInitialTsn),
		    remoteInitialTsn(remoteInitialTsn),
		    remoteAdvertisedReceiverWindowCredit(remoteAdvertisedReceiverWindowCredit),
		    tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities),
		    maxPacketLength(maxPacketLength),
		    isAssociationEstablished(std::move(isAssociationEstablished)),
		    t3RtxTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .label               = "sctp-t3-rtx",
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		        .maxRestarts         = std::nullopt })),
		    delayedAckTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .label               = "sctp-delayed-ack",
		        .baseTimeoutMs       = sctpOptions.delayedAckMaxTimeoutMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = std::nullopt,
		        .maxRestarts         = 0 })),
		    rto(sctpOptions),
		    txErrorCounter(sctpOptions),
		    // TODO: SCTP: Implement.
		    // dataTracker(),
		    reassemblyQueue(
		      sctpOptions.maxReceiverWindowBufferSize, negotiatedCapabilities.messageInterleaving),
		    retransmissionQueue(
		      this,
		      this->associationListener,
		      localInitialTsn,
		      remoteAdvertisedReceiverWindowCredit,
		      sendQueue,
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
		      std::addressof(this->reassemblyQueue),
		      std::addressof(this->retransmissionQueue)),
		    heartbeatHandler(this->associationListener, sctpOptions, this->shared, this)
		{
			MS_TRACE();

			sendQueue.EnableMessageInterleaving(this->negotiatedCapabilities.messageInterleaving);
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
			  std::unique_ptr<Packet>{ Packet::Factory(PacketFactoryBuffer, this->maxPacketLength) };

			packet->SetSourcePort(this->sctpOptions.sourcePort);
			packet->SetDestinationPort(this->sctpOptions.destinationPort);
			packet->SetVerificationTag(verificationTag);

			return packet;
		}

		bool TransmissionControlBlock::SendPacket(Packet* packet)
		{
			MS_TRACE();

			return this->packetSender.SendPacket(
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

			// SendPacket(packet.get());
		}

		void TransmissionControlBlock::MayAddForwardTsnChunk(Packet* packet, uint64_t nowMs)
		{
			MS_TRACE();

			if (nowMs >= this->limitForwardTsnUntilMs && this->retransmissionQueue.ShouldSendForwardTsn(nowMs))
			{
				if (this->negotiatedCapabilities.messageInterleaving)
				{
					this->retransmissionQueue.AddIForwardTsn(packet);
				}
				else
				{
					this->retransmissionQueue.AddForwardTsn(packet);
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
			auto result =
			  this->retransmissionQueue.GetChunksForFastRetransmit(packet->GetAvailableLength());

			for (auto& [tsn, data] : result)
			{
				if (this->negotiatedCapabilities.messageInterleaving)
				{
					auto* iDataChunk = packet->BuildChunkInPlace<IDataChunk>();

					iDataChunk->SetTsn(tsn);
					iDataChunk->SetUserData(std::move(data));
					iDataChunk->Consolidate();
				}
				else
				{
					auto* dataChunk = packet->BuildChunkInPlace<DataChunk>();

					dataChunk->SetTsn(tsn);
					dataChunk->SetUserData(std::move(data));
					dataChunk->Consolidate();
				}
			}

			SendPacket(packet.get());
		}

		void TransmissionControlBlock::SendBufferedPackets(Packet* packet, uint64_t nowMs)
		{
			MS_TRACE();

			for (size_t packetIdx{ 0 }; packetIdx < this->sctpOptions.maxBurst; ++packetIdx)
			{
				// Only add control Chunks to the first Packet that is sent, if sending
				// multiple Packets in one go (as allowed by the congestion window).
				if (packetIdx == 0)
				{
					if (this->remoteStateCookie.has_value())
					{
						// https://datatracker.ietf.org/doc/html/rfc9260#section-5.1
						//
						// "The COOKIE ECHO chunk can be bundled with any pending outbound
						// DATA chunks, but it MUST be the first chunk in the packet..."
						MS_ASSERT(packet->GetChunksCount() == 0, "Packet must have no Chunks");

						auto* cookieEchoChunk = packet->BuildChunkInPlace<CookieEchoChunk>();

						cookieEchoChunk->SetCookie(
						  remoteStateCookie->data(), static_cast<uint16_t>(remoteStateCookie->size()));
						cookieEchoChunk->Consolidate();
					}

					// https://datatracker.ietf.org/doc/html/rfc9260#section-6
					//
					// "Before an endpoint transmits a DATA chunk, if any received DATA
					// chunks have not been acknowledged (e.g., due to delayed ack), the
					// sender should create a SACK and bundle it with the outbound DATA
					// chunk, as long as the size of the final SCTP packet does not exceed
					// the current MTU."
					// TODO: SCTP: Implement it.
					// if (this->dataTracker.ShouldSendAck(/*alsoIffDelayed*/ true))
					// {
					//   builder.Add(this->dataTracker_.CreateSelectiveAck(
					//       reassembly_queue_.remaining_bytes()));
					// }

					const uint64_t nowMs = this->shared->GetTimeMs();

					MayAddForwardTsnChunk(packet, nowMs);

					if (this->streamResetHandler.ShouldSendStreamResetRequest())
					{
						this->streamResetHandler.AddStreamResetRequest(packet);
					}
				}

				auto chunksToSend =
				  this->retransmissionQueue.GetChunksToSend(nowMs, packet->GetAvailableLength());

				if (!chunksToSend.empty())
				{
					// https://datatracker.ietf.org/doc/html/rfc9260#section-8.3
					//
					// Sending DATA means that the path is not idle, restart heartbeat
					// timer.
					this->heartbeatHandler.RestartTimer();
				}

				const bool immediateAck =
				  GetCwnd() < (this->sctpOptions.immediateSackUnderCwndMtus * this->sctpOptions.mtu);

				for (auto& [tsn, data] : chunksToSend)
				{
					if (this->negotiatedCapabilities.messageInterleaving)
					{
						auto* dataChunk = packet->BuildChunkInPlace<DataChunk>();

						dataChunk->SetTsn(tsn);
						dataChunk->SetI(immediateAck);
						dataChunk->SetUserData(std::move(data));
						dataChunk->Consolidate();
					}
					else
					{
						auto* iDataChunk = packet->BuildChunkInPlace<IDataChunk>();

						iDataChunk->SetTsn(tsn);
						iDataChunk->SetI(immediateAck);
						iDataChunk->SetUserData(std::move(data));
						iDataChunk->Consolidate();
					}
				}

				// https://datatracker.ietf.org/doc/html/rfc9653#section-5.2
				//
				// "When an end point sends a packet containing a COOKIE ECHO chunk, it
				// MUST include a correct CRC32c checksum in the packet containing the
				// COOKIE ECHO chunk."
				if (!this->packetSender.SendPacket(
				      packet,
				      /*writeChecksum*/ !negotiatedCapabilities.zeroChecksum ||
				        this->remoteStateCookie.has_value()))
				{
					break;
				}

				if (this->remoteStateCookie.has_value())
				{
					// https://datatracker.ietf.org/doc/html/rfc9260#section-5.1
					//
					// "until the COOKIE ACK is returned the sender MUST NOT send any
					// other packets to the peer."
					break;
				}
			}
		}

		void TransmissionControlBlock::SendBufferedPackets(uint64_t nowMs)
		{
			MS_TRACE();

			auto packet = CreatePacket();

			SendBufferedPackets(packet.get(), nowMs);
		}

		void TransmissionControlBlock::OnT3RtxTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

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

					const uint64_t nowMs = this->shared->GetTimeMs();

					SendBufferedPackets(nowMs);
				}
			}
		}

		void TransmissionControlBlock::OnDelayedAckTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			// TODO: SCTP: Implement it.
			// this->dataTracker.HandleDelayedAckTimerExpiry();

			MaySendSackChunk();
		}

		void TransmissionControlBlock::OnBackoffTimer(
		  BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			const auto maxRestarts = backoffTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "%s timer has expired %zu/%s]",
			  backoffTimer->GetLabel().c_str(),
			  backoffTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

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
