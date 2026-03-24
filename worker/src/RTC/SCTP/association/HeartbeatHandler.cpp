#define MS_CLASS "RTC::SCTP::HeartbeatHandler"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "Logger.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		HeartbeatHandler::HeartbeatHandler(
		  AssociationListener& associationListener, const SctpOptions& sctpOptions, TCBContext* tcbContext)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    tcbContext(tcbContext),
		    intervalDurationMs(sctpOptions.heartbeatIntervalMs),
		    intervalDurationShouldIncludeRtt(sctpOptions.heartbeatIntervalIncludeRtt),
		    intervalTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.initialRtoMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeoutMs*/ sctpOptions.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ std::nullopt)),
		    timeoutTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.initialRtoMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::FIXED,
		        /*maxBackoffTimeoutMs*/ std::nullopt,
		        /*maxRestarts*/ 0))
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

		void HeartbeatHandler::ProcessReceivedHeartbeatRequestChunk(
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

		void HeartbeatHandler::ProcessReceivedHeartbeatAckChunk(
		  const HeartbeatAckChunk* receivedHeartbeatAckChunk)
		{
			MS_TRACE();

			this->timeoutTimer->Stop();

			// TODO
		}

		void HeartbeatHandler::OnIntervalTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->intervalTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "interval timer has expired %zu/%s]",
			  this->intervalTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO: STCP
		}

		void HeartbeatHandler::OnTimeoutTimer(uint64_t& /*baseTimeoutMs*/, bool& /*stop*/)
		{
			MS_TRACE();

			const auto maxRestarts = this->timeoutTimer->GetMaxRestarts();

			MS_DEBUG_TAG(
			  sctp,
			  "timeout timer has expired %zu/%s]",
			  this->timeoutTimer->GetExpirationCount(),
			  maxRestarts ? std::to_string(maxRestarts.value()).c_str() : "Infinite");

			// TODO: STCP
		}

		void HeartbeatHandler::OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
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
