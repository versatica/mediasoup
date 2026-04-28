#define MS_CLASS "RTC::SCTP::HeartbeatHandler"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		static constexpr int HeartbeatInfoLength{ 8 };

		/* Instance methods. */

		HeartbeatHandler::HeartbeatHandler(
		  AssociationListener& associationListener,
		  const SctpOptions& sctpOptions,
		  SharedInterface* shared,
		  TCBContext* tcbContext)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    shared(shared),
		    tcbContext(tcbContext),
		    intervalDurationMs(sctpOptions.heartbeatIntervalMs),
		    intervalDurationShouldIncludeRtt(sctpOptions.heartbeatIntervalIncludeRtt),
		    intervalTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::EXPONENTIAL,
		        .maxBackoffTimeoutMs = sctpOptions.timerMaxBackoffTimeoutMs,
		        .maxRestarts         = std::nullopt })),
		    timeoutTimer(this->shared->CreateBackoffTimer(
		      BackoffTimerHandleInterface::BackoffTimerHandleOptions{
		        .listener            = this,
		        .baseTimeoutMs       = sctpOptions.initialRtoMs,
		        .backoffAlgorithm    = BackoffTimerHandleInterface::BackoffAlgorithm::FIXED,
		        .maxBackoffTimeoutMs = std::nullopt,
		        .maxRestarts         = 0 }))
		{
			MS_TRACE();
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
				this->intervalTimer->SetBaseTimeoutMs(
				  this->intervalDurationMs + this->tcbContext->GetCurrentRtoMs());
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

			// Here we have to extract all Parameters from receivedHeartbeatRequestChunk
			// and add them into heartbeatAckChunk.
			for (auto it = receivedHeartbeatRequestChunk->ParametersBegin();
			     it != receivedHeartbeatRequestChunk->ParametersEnd();
			     ++it)
			{
				const auto* parameter = *it;

				heartbeatAckChunk->AddParameter(parameter);
			}

			heartbeatAckChunk->Consolidate();

			this->tcbContext->Send(packet.get());
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
				this->associationListener.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED,
				  "ignoring HEARTBEAT_ACK chunk without Heartbeat Info parameter");

				return;
			}

			const auto* info       = heartbeatInfoParameter->GetInfo();
			const uint16_t infoLen = heartbeatInfoParameter->GetInfoLength();

			if (!info)
			{
				this->associationListener.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "ignoring Heartbeat Info parameter without info field");

				return;
			}
			else if (infoLen != HeartbeatInfoLength)
			{
				this->associationListener.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "ignoring Heartbeat Info parameter with wrong length");

				return;
			}

			const uint64_t createdAtMs = Utils::Byte::Get8Bytes(info, 0);
			const uint64_t nowMs       = DepLibUV::GetTimeMs();

			if (createdAtMs > 0 && createdAtMs <= nowMs)
			{
				const uint64_t rttMs = nowMs - createdAtMs;

				MS_DEBUG_DEV("valid HEARTBEAT_ACK Chunk received, calling ObserveRttMs(%" PRIu64 ")", rttMs);

				this->tcbContext->ObserveRttMs(rttMs);
			}
			else
			{
				MS_WARN_DEV(
				  "ignoring received HEARTBEAT_ACK Chunk with invalid info content [createdAtMs:%" PRIu64
				  ", nowMs:%" PRIu64 "]",
				  createdAtMs,
				  nowMs);
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

			if (!this->tcbContext->IsAssociationEstablished())
			{
				MS_DEBUG_DEV("won't send HEARTBEAT_REQUEST when SCTP Association is not established");

				return;
			}

			const auto maxRestarts = this->intervalTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "interval timer has expired [%zu/%s]",
			  this->intervalTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			this->timeoutTimer->SetBaseTimeoutMs(this->tcbContext->GetCurrentRtoMs());
			this->timeoutTimer->Start();

			alignas(8) uint8_t info[HeartbeatInfoLength];
			const uint64_t nowMs = DepLibUV::GetTimeMs();

			Utils::Byte::Set8Bytes(info, 0, nowMs);

			auto packet                 = this->tcbContext->CreatePacket();
			auto* heartbeatRequestChunk = packet->BuildChunkInPlace<HeartbeatRequestChunk>();
			auto* heartbeatInfoParameter =
			  heartbeatRequestChunk->BuildParameterInPlace<HeartbeatInfoParameter>();

			heartbeatInfoParameter->SetInfo(info, HeartbeatInfoLength);
			heartbeatInfoParameter->Consolidate();
			heartbeatRequestChunk->Consolidate();

			MS_DEBUG_DEV("sending HEARTBEAT_REQUEST Chunk with info content [nowMs:%" PRIu64 "]", nowMs);

			this->tcbContext->Send(packet.get());
		}

		void HeartbeatHandler::OnTimeoutTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->timeoutTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "timeout timer has expired [%zu/%s]",
			  this->timeoutTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// Note that the timeout timer is not restarted. It will be started again when
			// the interval timer expires.
			MS_ASSERT(!this->timeoutTimer->IsRunning(), "timeout timer shouldn't be running");

			this->tcbContext->IncrementTxErrorCounter("hearbeat timeout");
		}

		void HeartbeatHandler::OnTimer(
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
