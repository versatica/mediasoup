#define MS_CLASS "RTC::SCTP::StreamResetHandler"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		StreamResetHandler::StreamResetHandler(
		  AssociationListener& associationListener, TCBContext* tcbContext
		  // TODO: SCTP: Implement
		  // DataTracker* dataTracker,
		  // ReassemblyQueue* reassemblyQueue,
		  // RetransmissionQueue* retransmissionQueue
		  )
		  : associationListener(associationListener),
		    tcbContext(tcbContext),
		    reconfigTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ 0,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeoutMs*/ std::nullopt,
		        /*maxRestarts*/ std::nullopt)),
		    nextOutgoingReqSeqNbr(tcbContext->GetLocalInitialTsn()),
		    lastProcessedReqSeqNbr(
		      this->incomingReconfigRequestSnUnwrapper.Unwrap(tcbContext->GetRemoteInitialTsn() - 1)),
		    lastProcessedReqResult(ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO)
		{
			MS_TRACE();
		}

		StreamResetHandler::~StreamResetHandler()
		{
			MS_TRACE();
		}

		// TODO

		void StreamResetHandler::OnReconfigTimer(uint64_t& baseTimeoutMs, bool& /*stop*/)
		{
			MS_TRACE();

			if (this->currentRequest && this->currentRequest->HasBeenSent())
			{
				if (this->currentRequest->IsDeferred())
				{
					// The request was deferred (received "In Progress"). This is not a
					// timeout, but just time to retry.
					this->currentRequest->SetDeferred(false);
				}
				else
				{
					// There is an outstanding request, which timed out while waiting for a
					// response.
					if (!this->tcbContext->IncrementTxErrorCounter("RECONFIG timeout"))
					{
						// Timed out. The connection will close after processing the timers.
						return;
					}
				}
			}
			else
			{
				// There is no outstanding request, but there is a prepared one. This means
				// that the receiver has previously responded "in progress", which resulted
				// in retrying the request (but with a new req_seq_nbr) after a while.
			}

			// TODO: SCTP: Do it.
			// ctx_->Send(ctx_->PacketBuilder().Add(MakeReconfigChunk()));

			baseTimeoutMs = this->tcbContext->GetCurrentRtoMs();
		}

		void StreamResetHandler::OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->reconfigTimer.get())
			{
				OnReconfigTimer(baseTimeoutMs, stop);
			}
		}
	} // namespace SCTP
} // namespace RTC
