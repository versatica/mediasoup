#define MS_CLASS "RTC::SCTP::HeartbeatHandler"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "Logger.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		HeartbeatHandler::HeartbeatHandler(
		  AssociationListener& associationListener, const SctpOptions& sctpOptions)
		  : associationListener(associationListener),
		    sctpOptions(sctpOptions),
		    intervalDurationMs(sctpOptions.heartbeatIntervalMs),
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

			// TODO: SCTP: Implement.
			// this->intervalTimer->SetBaseTimeoutMs(this->intervalDurationMs + ctx_->current_rto());
			this->intervalTimer->SetBaseTimeoutMs(this->intervalDurationMs);
			this->intervalTimer->Start();
		}

		void HeartbeatHandler::ProcessReceivedHeartbeatRequestChunk(
		  const HeartbeatRequestChunk* receivedHeartbeatRequestChunk)
		{
			MS_TRACE();

			// TODO
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
