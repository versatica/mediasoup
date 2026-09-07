#define MS_CLASS "RTC::SCTP::HeartbeatHandler"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "Utils.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		static constexpr int HeartbeatInfoLength{ 8 };

		/* Instance methods. */

		HeartbeatHandler::HeartbeatHandler(
		  AssociationListenerDeferrer& associationListenerDeferrer,
		  const SctpOptions& sctpOptions,
		  SharedInterface* shared,
		  TransmissionControlBlockContextInterface* tcbContext)
		  : associationListenerDeferrer(associationListenerDeferrer),
		    sctpOptions(sctpOptions),
		    shared(shared),
		    tcbContext(tcbContext),
		    intervalDurationMs(sctpOptions.heartbeatIntervalMs),
		    intervalDurationShouldIncludeRtt(sctpOptions.heartbeatIntervalIncludeRtt),
		    intervalTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .label               = "sctp-heartbeat-interval",
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::FIXED,
		        .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		        .maxRestarts         = std::nullopt })),
		    timeoutTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .label               = "sctp-heartbeat-timeout",
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = std::nullopt,
		        .maxRestarts         = 0 }))
		{
			MS_TRACE();

			// The interval timer must always be running as long as the association
			// is up (so when the TCB is created, which is the one that creates the
			// HeartbeatHandler.
			RestartTimer();
		}

		HeartbeatHandler::~HeartbeatHandler()
		{
			MS_TRACE();
		}

		void HeartbeatHandler::RestartTimer()
		{
			MS_TRACE();

			// Heartbeating has been disabled.
			if (this->intervalDurationMs == 0)
			{
				return;
			}

			if (intervalDurationShouldIncludeRtt)
			{
				// NOTE: The timer takes milliseconds, so the RTO is truncated here.
				this->intervalTimer->SetBaseTimeoutMs(
				  this->intervalDurationMs +
				  static_cast<uint64_t>(this->tcbContext->GetCurrentRtoUs() / 1000));
			}
			else
			{
				this->intervalTimer->SetBaseTimeoutMs(this->intervalDurationMs);
			}

			this->intervalTimer->Start();
		}

		void HeartbeatHandler::HandleReceivedHeartbeatRequestChunk(
		  const HeartbeatRequestChunk* receivedHeartbeatRequestChunk)
		{
			MS_TRACE();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.3
			//
			// "The receiver of the HEARTBEAT chunk SHOULD immediately respond with a
			// HEARTBEAT ACK chunk that contains the Heartbeat Information TLV,
			// together with any other received TLVs, copied unchanged from the
			// received HEARTBEAT chunk."
			auto packet             = this->tcbContext->CreatePacket();
			auto* heartbeatAckChunk = packet->BuildChunkInPlace<HeartbeatAckChunk>();

			// Here we have to extract all parameters from receivedHeartbeatRequestChunk
			// and add them into heartbeatAckChunk.
			for (auto it = receivedHeartbeatRequestChunk->ParametersBegin();
			     it != receivedHeartbeatRequestChunk->ParametersEnd();
			     ++it)
			{
				const auto* parameter = *it;

				heartbeatAckChunk->AddParameter(parameter);
			}

			heartbeatAckChunk->Consolidate();

			this->tcbContext->SendPacket(packet.get());
		}

		void HeartbeatHandler::HandleReceivedHeartbeatAckChunk(
		  const HeartbeatAckChunk* receivedHeartbeatAckChunk)
		{
			MS_TRACE();

			this->timeoutTimer->Stop();

			const auto* heartbeatInfoParameter =
			  receivedHeartbeatAckChunk->GetFirstParameterOfType<HeartbeatInfoParameter>();

			if (!heartbeatInfoParameter)
			{
				this->associationListenerDeferrer.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED,
				  "ignoring HEARTBEAT-ACK chunk without Heartbeat Info parameter");

				return;
			}

			const auto* info       = heartbeatInfoParameter->GetInfo();
			const uint16_t infoLen = heartbeatInfoParameter->GetInfoLength();

			if (!info)
			{
				this->associationListenerDeferrer.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "ignoring Heartbeat Info parameter without info field");

				return;
			}
			else if (infoLen != HeartbeatInfoLength)
			{
				this->associationListenerDeferrer.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "ignoring Heartbeat Info parameter with wrong length");

				return;
			}

			// NOTE: The peer echoes back the info we wrote, so this value cannot be
			// trusted. The guard below rejects it unless it's a past instant, which
			// also rejects a negative resulting from garbage above 2^63.
			const int64_t createdAtUs = static_cast<int64_t>(Utils::Byte::Get8Bytes(info, 0));
			const int64_t nowUs       = this->shared->GetTimeUsInt64();

			if (createdAtUs > 0 && createdAtUs <= nowUs)
			{
				const int64_t rttUs = nowUs - createdAtUs;

				MS_DEBUG_DEV("valid HEARTBEAT-ACK chunk received, calling ObserveRttUs(%" PRIi64 ")", rttUs);

				this->tcbContext->ObserveRttUs(rttUs);
			}
			else
			{
				MS_WARN_DEV(
				  "ignoring received HEARTBEAT-ACK chunk with invalid info content [createdAtUs:%" PRIi64
				  ", nowUs:%" PRIi64 "]",
				  createdAtUs,
				  nowUs);
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.1
			//
			// "When a HEARTBEAT ACK chunk is received from the peer endpoint, the
			// counter SHOULD also be reset."
			this->tcbContext->ClearTxErrorCounter();
		}

		void HeartbeatHandler::OnIntervalTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
			const auto maxRestarts = this->intervalTimer->GetMaxRestarts();
#endif

			// NOTE: This timer expires periodically on idle connections (forever), so
			// it's logged at dev level to avoid being noisy.
			MS_DEBUG_DEV(
			  "%s timer has expired [expirations:%zu, maxRestarts:%s]",
			  this->intervalTimer->GetLabel().c_str(),
			  this->intervalTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// This is a top-level timer entry point (invoked by libuv outside any other
			// SCTP API call), so it must establish the deferrer scope itself, just like
			// Association does in its own timer handlers.
			const AssociationListenerDeferrer::ScopedDeferrer deferrer(this->associationListenerDeferrer);

			if (!this->tcbContext->IsAssociationEstablished())
			{
				MS_DEBUG_DEV("won't send HEARTBEAT-REQUEST when SCTP association is not established");

				return;
			}

			// NOTE: The timer takes milliseconds, so the RTO is truncated here.
			this->timeoutTimer->SetBaseTimeoutMs(
			  static_cast<uint64_t>(this->tcbContext->GetCurrentRtoUs() / 1000));
			this->timeoutTimer->Start();

			alignas(8) uint8_t info[HeartbeatInfoLength];

			// NOTE: This is read back in HandleReceivedHeartbeatAckChunk() when the
			// peer echoes it, so both sides of it must use the same unit.
			const int64_t nowUs = this->shared->GetTimeUsInt64();

			Utils::Byte::Set8Bytes(info, 0, static_cast<uint64_t>(nowUs));

			auto packet                 = this->tcbContext->CreatePacket();
			auto* heartbeatRequestChunk = packet->BuildChunkInPlace<HeartbeatRequestChunk>();
			auto* heartbeatInfoParameter =
			  heartbeatRequestChunk->BuildParameterInPlace<HeartbeatInfoParameter>();

			heartbeatInfoParameter->SetInfo(info, HeartbeatInfoLength);
			heartbeatInfoParameter->Consolidate();
			heartbeatRequestChunk->Consolidate();

			MS_DEBUG_DEV("sending HEARTBEAT-REQUEST chunk with info content [nowUs:%" PRIi64 "]", nowUs);

			this->tcbContext->SendPacket(packet.get());
		}

		void HeartbeatHandler::OnTimeoutTimer(uint64_t& /*baseTimeoutMs*/, bool& stop)
		{
			MS_TRACE();

			const auto maxRestarts = this->timeoutTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "%s timer has expired [expirations:%zu, maxRestarts:%s]",
			  this->timeoutTimer->GetLabel().c_str(),
			  this->timeoutTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// This is a top-level timer entry point (invoked by libuv outside any other
			// SCTP API call), so it must establish the deferrer scope itself, just like
			// Association does in its own timer handlers.
			const AssociationListenerDeferrer::ScopedDeferrer deferrer(this->associationListenerDeferrer);

			// Note that the timeout timer is not restarted. It will be started again when
			// the interval timer expires.
			MS_ASSERT(!this->timeoutTimer->IsRunning(), "timeout timer shouldn't be running");

			if (!this->tcbContext->IncrementTxErrorCounter("hearbeat timeout"))
			{
				// `IncrementTxErrorCounter()` has closed (and destroyed) the TCB (and
				// hence this HeartbeatHandler and its timers). Signal the firing timer to
				// stop and don't touch any member afterwards.
				stop = true;

				return;
			}
		}

		void HeartbeatHandler::OnBackoffTimer(
		  BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->intervalTimer.get())
			{
				OnIntervalTimer(baseTimeoutMs, stop);
			}
			else if (backoffTimer == this->timeoutTimer.get())
			{
				OnTimeoutTimer(baseTimeoutMs, stop);
			}
		}
	} // namespace SCTP
} // namespace RTC
