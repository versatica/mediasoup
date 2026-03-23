#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <cmath> // std::min()

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
		  PacketSender& packetSender,
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    packetSender(packetSender),
		    localVerificationTag(localVerificationTag),
		    remoteVerificationTag(remoteVerificationTag),
		    localInitialTsn(localInitialTsn),
		    remoteInitialTsn(remoteInitialTsn),
		    remoteAdvertisedReceiverWindowCredit(remoteAdvertisedReceiverWindowCredit),
		    tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities),
		    t3RtxTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.initialRtoMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeoutMs*/ sctpOptions.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ std::nullopt)),
		    delayedAckTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.delayedAckMaxTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeoutMs*/ std::nullopt,
		        /*maxRestarts*/ 0)),
		    rto(sctpOptions),
		    txErrorCounter(sctpOptions)
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

		void TransmissionControlBlock::ObserveRtt(uint64_t rtt)
		{
			MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
			const auto prevRtoMs = this->rto.GetRtoMs();
#endif

			this->rto.ObserveRtt(rtt);

			MS_DEBUG_DEV(
			  "new rtt:%" PRIu64 ", previous rto:%" PRIu64 ", new rto:%" PRIu64 ", srtt:%" PRIu64,
			  rtt,
			  prevRtoMs,
			  this->rto.GetRtoMs(),
			  this - rto.GetSrttMs());

			this->t3RtxTimer->SetBaseTimeoutMs(this->rto.GetRtoMs());
			this->t3RtxTimer->Start();

			const uint64_t delayedAckTimeoutMs = std::min(
			  static_cast<uint64_t>(this->rto.GetRtoMs() * 0.5), this->sctpOptions.delayedAckMaxTimeoutMs);

			this->delayedAckTimer->SetBaseTimeoutMs(delayedAckTimeoutMs);
			this->delayedAckTimer->Start();
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

			// TODO: Implement it.
			// if (this->dataTracker.ShouldSendAckChunk(/*alsoIfDelayed*/ false))
			// {
			// 	auto packet = CreatePacket();

			// 	// TODO: Here we must create a SackChunk in the Packet, however the
			// 	// SackChunk is in theory generated by this->dataTracker... Let's see.
			// 	builder.Add(this->dataTracker.CreateSelectiveAck(this->reassemblyQueue.GetRemainingBytes()));

			// 	Send(packet.get());
			// }
		}

		void TransmissionControlBlock::Send(Packet* packet)
		{
			MS_TRACE();

			this->packetSender.SendPacket(
			  packet,
			  /*writeChecksum*/ !this->negotiatedCapabilities.zeroChecksum);
		}

		void TransmissionControlBlock::OnT3RtxTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->t3RtxTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "T3-rtx timer has expired %zu/%s]",
			  this->t3RtxTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// In the COOKIE_ECHO state, let the T1-COOKIE timer trigger
			// retransmissions, to avoid having two timers doing that.
			// TODO: Implement it.
			// if (this->cookieEchoChunk.has_value())
			// {
			// 	MS_DEBUG_DEV("not retransmitting as T1-cookie is active");
			// }
			// else
			// {
			// 	if (IncrementTxErrorCounter("t3-rtx expired"))
			// 	{
			// 		this->retransmissionQueue.HandleT3RtxTimerExpiry();

			// 		const uint64_t now = DepLibUV::GetTimeMs();

			// 		SendBufferedPackets(now);
			// 	}
			// }
		}

		void TransmissionControlBlock::OnDelayedAckTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->delayedAckTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "delayer ack timer has expired %zu/%s]",
			  this->delayedAckTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO: Implement it.
			// this->dataTracker.HandleDelayedAckTimerExpiry();

			// TODO: Implement it.
			// MaySendSackChunk();
		}

		void TransmissionControlBlock::OnTimer(
		  BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
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
	} // namespace SCTP
} // namespace RTC
