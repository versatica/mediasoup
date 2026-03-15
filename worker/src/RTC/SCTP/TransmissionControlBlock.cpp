#include <cstdint>
#define MS_CLASS "RTC::SCTP::TransmissionControlBlock"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "RTC/SCTP/TransmissionControlBlock.hpp"
#include <cmath> // std::min()

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		thread_local static uint8_t PacketFactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

		/* Instance methods. */

		TransmissionControlBlock::TransmissionControlBlock(
		  SocketListener& listener,
		  const SctpOptions& sctpOptions,
		  PacketSender& packetSender,
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		  : listener(listener),
		    sctpOptions(sctpOptions),
		    packetSender(packetSender),
		    localVerificationTag(localVerificationTag),
		    remoteVerificationTag(remoteVerificationTag),
		    localInitialTsn(localInitialTsn),
		    remoteInitialTsn(remoteInitialTsn),
		    remoteAdvertisedReceiverWindowCredit(remoteAdvertisedReceiverWindowCredit),
		    tieTag(tieTag),
		    negotiatedCapabilities(negotiatedCapabilities),
		    rto(sctpOptions),
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
		        /*maxRestarts*/ 0))
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
			MS_DUMP_CLEAN(indentation, "</SCTP::TransmissionControlBlock>");
		}

		void TransmissionControlBlock::ObserveRtt(uint64_t rtt)
		{
			MS_TRACE();

			const auto prevRtoMs = this->rto.GetRtoMs();

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

		void TransmissionControlBlock::OnT3RtxTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			const auto maxRestarts = this->t3RtxTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "T3-rtx timer has expired %zu/%s]",
			  this->t3RtxTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO
		}

		void TransmissionControlBlock::OnDelayedAckTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			const auto maxRestarts = this->delayedAckTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "delayer ack timer has expired %zu/%s]",
			  this->delayedAckTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO
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
