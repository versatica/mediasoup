// #define MS_CLASS "RTC::SCTP::RetransmissionQueue"
// // TODO: SCTP: COMMENT
// #define MS_LOG_DEV_LEVEL 3

// #include "RTC/SCTP/association/RetransmissionQueue.hpp"
// #include "Logger.hpp"
// #include "RTC/Consts.hpp"
// #include "RTC/SCTP/packet/Parameter.hpp"
// #include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"

// namespace RTC
// {
// 	namespace SCTP
// 	{
// 		/* Instance methods. */

// 		RetransmissionQueue::RetransmissionQueue(
// 		  AssociationListener& associationListener, TCBContext* tcbContext
// 		  // TODO: SCTP: Implement
// 		  // DataTracker* dataTracker,
// 		  // ReassemblyQueue* reassemblyQueue,
// 		  // RetransmissionQueue* retransmissionQueue
// 		  )
// 		  : associationListener(associationListener),
// 		    tcbContext(tcbContext),
// 		    reConfigTimer(
// 		      std::make_unique<BackoffTimerHandle>(
// 		        /*listener*/ this,
// 		        /*baseTimeoutMs*/ 0,
// 		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
// 		        /*maxBackoffTimeoutMs*/ std::nullopt,
// 		        /*maxRestarts*/ std::nullopt)),
// 		    nextOutgoingReqSeqNbr(tcbContext->GetLocalInitialTsn()),
// 		    lastProcessedReqSeqNbr(
// 		      this->incomingReConfigRequestSnUnwrapper.Unwrap(tcbContext->GetRemoteInitialTsn() - 1)),
// 		    lastProcessedReqResult(ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO)
// 		{
// 			MS_TRACE();
// 		}

// 		StreamResetHandler::~StreamResetHandler()
// 		{
// 			MS_TRACE();
// 		}
// 	}
// }
