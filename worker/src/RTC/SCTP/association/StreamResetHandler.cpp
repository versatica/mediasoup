#define MS_CLASS "RTC::SCTP::StreamResetHandler"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		alignas(4) thread_local static uint8_t ChunkFactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

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
		    reConfigTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ 0,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeoutMs*/ std::nullopt,
		        /*maxRestarts*/ std::nullopt)),
		    nextOutgoingReqSeqNbr(tcbContext->GetLocalInitialTsn()),
		    lastProcessedReqSeqNbr(
		      this->incomingReConfigRequestSnUnwrapper.Unwrap(tcbContext->GetRemoteInitialTsn() - 1)),
		    lastProcessedReqResult(ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO)
		{
			MS_TRACE();
		}

		StreamResetHandler::~StreamResetHandler()
		{
			MS_TRACE();
		}

		void StreamResetHandler::ResetStreams(std::span<const uint16_t> /*outgoingStreamIds*/)
		{
			MS_TRACE();

			// TODO: SCTP: Uncomment.
			// for (const auto streamId : outgoingStreamIds)
			{
				// TODO: SCTP: Implement it.
				// this->retransmissionQueue->PrepareResetStream(streamId);
			}
		}

		void StreamResetHandler::HandleReceivedReConfigChunk(const ReConfigChunk* receivedReConfigChunk)
		{
			MS_TRACE();

			if (!ValidateReceivedReConfigChunk(receivedReConfigChunk))
			{
				this->associationListener.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "invalid RE-CONFIG command received");

				return;
			}

			auto packet         = this->tcbContext->CreatePacket();
			auto* reConfigChunk = packet->BuildChunkInPlace<ReConfigChunk>();

			for (auto it = receivedReConfigChunk->ParametersBegin();
			     it != receivedReConfigChunk->ParametersEnd();
			     ++it)
			{
				const auto* parameter = *it;

				switch (parameter->GetType())
				{
					case Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST:
					{
						HandleReceivedOutgoingSsnResetRequestParameter(
						  reinterpret_cast<const OutgoingSsnResetRequestParameter*>(parameter), reConfigChunk);

						break;
					}

					case Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST:
					{
						HandleReceivedIncomingSsnResetRequestParameter(
						  reinterpret_cast<const IncomingSsnResetRequestParameter*>(parameter), reConfigChunk);

						break;
					}

					case Parameter::ParameterType::RECONFIGURATION_RESPONSE:
					{
						HandleReceivedReconfigurationResponseParameter(
						  reinterpret_cast<const ReconfigurationResponseParameter*>(parameter));

						break;
					}

					default:;
				}
			}

			reConfigChunk->Consolidate();

			if (reConfigChunk->GetParametersCount() > 0)
			{
				this->tcbContext->Send(packet.get());
			}
		}

		bool StreamResetHandler::ValidateReceivedReConfigChunk(const ReConfigChunk* receivedReConfigChunk)
		{
			MS_TRACE();

			if (receivedReConfigChunk->GetParametersCount() == 1)
			{
				const auto* firstParameter = receivedReConfigChunk->GetParameterAt(0);

				if (
				  firstParameter->GetType() == Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST ||
				  firstParameter->GetType() == Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST ||
				  firstParameter->GetType() == Parameter::ParameterType::SSN_TSN_RESET_REQUEST ||
				  firstParameter->GetType() == Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST ||
				  firstParameter->GetType() == Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST ||
				  firstParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE)
				{
					return true;
				}
			}
			else if (receivedReConfigChunk->GetParametersCount() == 2)
			{
				const auto* firstParameter  = receivedReConfigChunk->GetParameterAt(0);
				const auto* secondParameter = receivedReConfigChunk->GetParameterAt(1);

				if (
				  (firstParameter->GetType() == Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST &&
				   secondParameter->GetType() == Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST) ||
				  (firstParameter->GetType() == Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST &&
				   secondParameter->GetType() == Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST) ||
				  (firstParameter->GetType() == Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST &&
				   secondParameter->GetType() == Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST) ||
				  (firstParameter->GetType() == Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST &&
				   secondParameter->GetType() == Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST) ||
				  (firstParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE &&
				   secondParameter->GetType() == Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST) ||
				  (firstParameter->GetType() == Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST &&
				   secondParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE) ||
				  (firstParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE &&
				   secondParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE) ||
				  (firstParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE &&
				   secondParameter->GetType() == Parameter::ParameterType::RECONFIGURATION_RESPONSE))
				{
					return true;
				}
			}

			MS_WARN_TAG(sctp, "invalid set of RE-CONFIG Parameters");

			return false;
		}

		ReConfigChunk* StreamResetHandler::CreateStreamResetRequest()
		{
			MS_TRACE();

			// Only send stream resets if there are streams to reset, and no current
			// ongoing request (there can only be one at a time), and if the stream
			// can be reset.
			// TODO: SCTP: Implement it.
			// if (this->currentRequest.has_value() ||
			//     !this->retransmissionQueue->HasStreamsReadyToBeReset())
			// {
			//   return nullptr;
			// }

			// TODO: SCTP: Implement it.
			// this->currentRequest.emplace(
			//   this->retransmissionQueue->GetLastAssignedTsn(),
			//   this->retransmissionQueue->BeginResetStreams());

			this->reConfigTimer->SetBaseTimeoutMs(this->tcbContext->GetCurrentRtoMs());
			this->reConfigTimer->Start();

			return CreateReconfigChunk();
		}

		ReConfigChunk* StreamResetHandler::CreateReconfigChunk()
		{
			MS_TRACE();

			// The `reqSeqNbr` will be empty if the request has never been sent before,
			// or if it was sent, but the sender responded "in progress", and then the
			// `reqSeqNbr` will be cleared to re-send with a new number. But if the
			// request is re-sent due to timeout (re-config timer expiring), the same
			// `reqSeqNbr` will be used.
			MS_ASSERT(this->currentRequest.has_value(), "currentRequest optional must have value");

			if (this->currentRequest->HasBeenSent())
			{
				this->currentRequest->PrepareToSend(this->nextOutgoingReqSeqNbr);
				this->nextOutgoingReqSeqNbr = uint32_t{ this->nextOutgoingReqSeqNbr + 1 };
			}

			auto* reConfigChunk = ReConfigChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));
			auto* outgoingSsnResetRequestParameter =
			  reConfigChunk->BuildParameterInPlace<OutgoingSsnResetRequestParameter>();

			outgoingSsnResetRequestParameter->SetReconfigurationRequestSequenceNumber(
			  this->currentRequest->GetReqSeqNbr());
			outgoingSsnResetRequestParameter->SetReconfigurationResponseSequenceNumber(
			  this->currentRequest->GetReqSeqNbr());
			outgoingSsnResetRequestParameter->SetSenderLastAssignedTsn(
			  this->currentRequest->GetSenderLastAssignedTsn());

			for (const auto& streamId : this->currentRequest->GetStreamIds())
			{
				outgoingSsnResetRequestParameter->AddStreamId(streamId);
			}

			outgoingSsnResetRequestParameter->Consolidate();

			return reConfigChunk;
		}

		StreamResetHandler::ReqSeqNbrValidationResult StreamResetHandler::ValidateReqSeqNbr(
		  StreamResetHandler::UnwrappedReConfigRequestSn reqSeqNbr)
		{
			MS_TRACE();

			if (reqSeqNbr == this->lastProcessedReqSeqNbr)
			{
				return ReqSeqNbrValidationResult::RETRANSMISSION;
			}
			else if (reqSeqNbr != this->lastProcessedReqSeqNbr.GetNextValue())
			{
				// Too old, too new, from wrong Association, etc.
				MS_WARN_TAG(sctp, "bad reqSeqNbr: %" PRIu32, reqSeqNbr.Wrap());

				return ReqSeqNbrValidationResult::BAD_SEQUENCE_NUMBER;
			}
			else
			{
				return ReqSeqNbrValidationResult::VALID;
			}
		}

		void StreamResetHandler::HandleReceivedOutgoingSsnResetRequestParameter(
		  const OutgoingSsnResetRequestParameter* receivedOutgoingSsnResetRequestParameter,
		  ReConfigChunk* reConfigChunk)
		{
			MS_TRACE();

			const UnwrappedReConfigRequestSn requestSn = this->incomingReConfigRequestSnUnwrapper.Unwrap(
			  receivedOutgoingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
			const ReqSeqNbrValidationResult validationResult = ValidateReqSeqNbr(requestSn);

			if (validationResult == ReqSeqNbrValidationResult::BAD_SEQUENCE_NUMBER)
			{
				auto* reconfigurationResponseParameter =
				  reConfigChunk->BuildParameterInPlace<ReconfigurationResponseParameter>();

				reconfigurationResponseParameter->SetReconfigurationResponseSequenceNumber(
				  receivedOutgoingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
				reconfigurationResponseParameter->SetResult(
				  ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER);

				reconfigurationResponseParameter->Consolidate();

				return;
			}

			// If this is a retransmission of a request that has already been finalized
			// (i.e., not "In Progress"), just send the previous final response.
			if (
			  validationResult == ReqSeqNbrValidationResult::RETRANSMISSION &&
			  this->lastProcessedReqResult != ReconfigurationResponseParameter::Result::IN_PROGRESS)
			{
				auto* reconfigurationResponseParameter =
				  reConfigChunk->BuildParameterInPlace<ReconfigurationResponseParameter>();

				reconfigurationResponseParameter->SetReconfigurationResponseSequenceNumber(
				  receivedOutgoingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
				reconfigurationResponseParameter->SetResult(this->lastProcessedReqResult);

				reconfigurationResponseParameter->Consolidate();

				return;
			}

			// At this point, the request is either brand new, a buggy client sending
			// a new SN after "In Progress", or a compliant client retransmitting an
			// "In Progress" request. In all cases, re-evaluate the state.
			this->lastProcessedReqSeqNbr = requestSn;

			// // TODO: SCTP implement.
			// if (this->dataTracker->IsLaterThanCumulativeAckedTsn(
			//         receivedOutgoingSsnResetRequestParameter->GetSenderLastAssignedTsn()))
			// {
			//   // https://datatracker.ietf.org/doc/html/rfc6525#section-5.2.2
			//   //
			//   // E2) "If the Sender's Last Assigned TSN is greater than the cumulative
			//   // acknowledgment point, then the endpoint MUST enter 'deferred reset
			//   // processing'."
			//   this->reassemblyQueue->EnterDeferredReset(
			//   	receivedOutgoingSsnResetRequestParameter->GetSenderLastAssignedTsn(),
			//   	receivedOutgoingSsnResetRequestParameter->GetStreamIds());

			//   // "If the endpoint enters 'deferred reset processing', it MUST put a
			//   // Re-configuration Response Parameter into a RE-CONFIG chunk indicating
			//   // 'In progress' and MUST send the RE-CONFIG chunk.
			//   this->lastProcessedReqResult = ReconfigurationResponseParameter::Result::IN_PROGRESS;

			//  	MS_DEBUG_DEV("reset outgoing in progress, sender last assigned tsn %" PRIu32 " not yet
			//  reached", receivedOutgoingSsnResetRequestParameter->GetSenderLastAssignedTsn());
			// } else {
			//   // https://datatracker.ietf.org/doc/html/rfc6525#section-5.2.2
			//   //
			//   // E3) If no stream numbers are listed in the parameter, then all incoming
			//   // streams MUST be reset to 0 as the next expected SSN. If specific stream
			//   // numbers are listed, then only these specific streams MUST be reset to
			//   // 0, and all other non-listed SSNs remain unchanged. E4: Any queued TSNs
			//   // (queued at step E2) MUST now be released and processed normally.
			//   this->reassemblyQueue->ResetStreamsAndLeaveDeferredReset(receivedOutgoingSsnResetRequestParameter->GetStreamIds());

			//   this->associationListener.OnAssociationInboundStreamsReset(receivedOutgoingSsnResetRequestParameter->GetStreamIds());

			//   this->lastProcessedReqResult = ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED;

			//   MS_DEBUG_DEV("reset outgoing performed");
			//  	MS_DEBUG_DEV("reset outgoing performed, sender last assigned tsn %" PRIu32 " reached",
			//  receivedOutgoingSsnResetRequestParameter->GetSenderLastAssignedTsn());
			// }

			auto* reconfigurationResponseParameter =
			  reConfigChunk->BuildParameterInPlace<ReconfigurationResponseParameter>();

			reconfigurationResponseParameter->SetReconfigurationResponseSequenceNumber(
			  receivedOutgoingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
			reconfigurationResponseParameter->SetResult(this->lastProcessedReqResult);

			reconfigurationResponseParameter->Consolidate();
		}

		void StreamResetHandler::HandleReceivedIncomingSsnResetRequestParameter(
		  const IncomingSsnResetRequestParameter* receivedIncomingSsnResetRequestParameter,
		  ReConfigChunk* reConfigChunk)
		{
			MS_TRACE();

			const UnwrappedReConfigRequestSn requestSn = this->incomingReConfigRequestSnUnwrapper.Unwrap(
			  receivedIncomingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
			const ReqSeqNbrValidationResult validationResult = ValidateReqSeqNbr(requestSn);

			if (validationResult == ReqSeqNbrValidationResult::VALID || validationResult == ReqSeqNbrValidationResult::RETRANSMISSION)
			{
				auto* reconfigurationResponseParameter =
				  reConfigChunk->BuildParameterInPlace<ReconfigurationResponseParameter>();

				reconfigurationResponseParameter->SetReconfigurationResponseSequenceNumber(
				  receivedIncomingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
				reconfigurationResponseParameter->SetResult(
				  ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO);

				reconfigurationResponseParameter->Consolidate();
			}
			else
			{
				auto* reconfigurationResponseParameter =
				  reConfigChunk->BuildParameterInPlace<ReconfigurationResponseParameter>();

				reconfigurationResponseParameter->SetReconfigurationResponseSequenceNumber(
				  receivedIncomingSsnResetRequestParameter->GetReconfigurationRequestSequenceNumber());
				reconfigurationResponseParameter->SetResult(
				  ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER);

				reconfigurationResponseParameter->Consolidate();
			}
		}

		void StreamResetHandler::HandleReceivedReconfigurationResponseParameter(
		  const ReconfigurationResponseParameter* receivedReconfigurationResponseParameter)

		{
			MS_TRACE();

			if (
			  this->currentRequest.has_value() && this->currentRequest->HasBeenSent() &&
			  receivedReconfigurationResponseParameter->GetReconfigurationResponseSequenceNumber() ==
			    this->currentRequest->GetReqSeqNbr())
			{
				this->reConfigTimer->Stop();

				switch (receivedReconfigurationResponseParameter->GetResult())
				{
					case RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO:
					case RTC::SCTP::ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED:
					{
						MS_DEBUG_DEV(
						  "reset stream success [reqSeqNbr:%" PRIu32 "]", this->currentRequest->GetReqSeqNbr());

						this->associationListener.OnAssociationStreamsResetPerformed(
						  this->currentRequest->GetStreamIds());

						this->currentRequest = std::nullopt;

						// TODO: SCTP: Implement it.
						// this->retransmissionQueue->CommitResetSteam();

						break;
					}

					case RTC::SCTP::ReconfigurationResponseParameter::Result::IN_PROGRESS:
					{
						MS_DEBUG_DEV(
						  "reset stream still pending [reqSeqNbr:%" PRIu32 "]",
						  this->currentRequest->GetReqSeqNbr());

						// Force this request to be sent again, but with the same `reqSeqNbr`.
						this->currentRequest->SetDeferred(true);

						this->reConfigTimer->SetBaseTimeoutMs(this->tcbContext->GetCurrentRtoMs());
						this->reConfigTimer->Start();

						break;
					}

					case RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS:
					case RTC::SCTP::ReconfigurationResponseParameter::Result::DENIED:
					case RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_WRONG_SSN:
					case RTC::SCTP::ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER:
					{
						MS_WARN_TAG(
						  sctp,
						  "reset stream error [reqSeqNbr:%" PRIu32 ", result:%s]",
						  this->currentRequest->GetReqSeqNbr(),
						  ReconfigurationResponseParameter::ResultToString(
						    receivedReconfigurationResponseParameter->GetResult())
						    .c_str());

						this->associationListener.OnAssociationStreamsResetFailed(
						  this->currentRequest->GetStreamIds(),
						  ReconfigurationResponseParameter::ResultToString(
						    receivedReconfigurationResponseParameter->GetResult()));

						this->currentRequest = std::nullopt;

						// TODO: SCTP: Implement it.
						// this->retransmissionQueue->RollbackResetStreams();

						break;
					}
				}
			}
		}

		void StreamResetHandler::OnReConfigTimer(uint64_t& baseTimeoutMs, bool& /*stop*/)
		{
			MS_TRACE();

			if (this->currentRequest && this->currentRequest->HasBeenSent())
			{
				// The request was deferred (received "In Progress"). This is not a
				// timeout, but just time to retry.
				if (this->currentRequest->IsDeferred())
				{
					this->currentRequest->SetDeferred(false);
				}
				// There is an outstanding request, which timed out while waiting for a
				// response.
				else if (!this->tcbContext->IncrementTxErrorCounter("RECONFIG timeout"))
				{
					// Timed out. The connection will close after processing the timers.
					return;
				}
			}
			else
			{
				// There is no outstanding request, but there is a prepared one. This means
				// that the receiver has previously responded "in progress", which resulted
				// in retrying the request (but with a new `reqSeqNbr`) after a while.
			}

			auto packet = this->tcbContext->CreatePacket();

			packet->AddChunk(CreateReconfigChunk());

			this->tcbContext->Send(packet.get());

			baseTimeoutMs = this->tcbContext->GetCurrentRtoMs();
		}

		void StreamResetHandler::OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->reConfigTimer.get())
			{
				OnReConfigTimer(baseTimeoutMs, stop);
			}
		}
	} // namespace SCTP
} // namespace RTC
