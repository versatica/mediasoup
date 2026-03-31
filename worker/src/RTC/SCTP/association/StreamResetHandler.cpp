#define MS_CLASS "RTC::SCTP::StreamResetHandler"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "Logger.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

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
		      this->incomingReconfigRequestSnUnwrapper.Unwrap(tcbContext->GetRemoteInitialTsn() - 1)),
		    lastProcessedReqResult(ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO)
		{
			MS_TRACE();
		}

		StreamResetHandler::~StreamResetHandler()
		{
			MS_TRACE();
		}

		void StreamResetHandler::ResetStreams(std::span<const uint16_t> outgoingStreamIds)
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

			std::optional<std::vector<ReconfigurationResponseParameter>> responses =
			  ProcessReceivedReConfigChunk(receivedReConfigChunk);

			if (!responses.has_value())
			{
				this->associationListener.OnAssociationError(
				  Types::ErrorKind::PARSE_FAILED, "failed to parse RE-CONFIG command");

				return;
			}

			if (responses->empty())
			{
				return;
			}

			auto packet         = this->tcbContext->CreatePacket();
			auto* reConfigChunk = packet->BuildChunkInPlace<ReConfigChunk>();

			for (const auto& response : responses.value())
			{
				reConfigChunk->AddParameter(std::addressof(response));
			}

			reConfigChunk->Consolidate();

			this->tcbContext->Send(packet.get());
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

		std::optional<std::vector<ReconfigurationResponseParameter>> StreamResetHandler::ProcessReceivedReConfigChunk(
		  const ReConfigChunk* receivedReConfigChunk)
		{
			MS_TRACE();

			if (!ValidateReceivedReConfigChunk(receivedReConfigChunk))
			{
				return std::nullopt;
			}

			std::vector<ReconfigurationResponseParameter> responses;

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
						  reinterpret_cast<const OutgoingSsnResetRequestParameter*>(parameter), responses);

						break;
					}

					case Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST:
					{
						HandleReceivedIncomingSsnResetRequestParameter(
						  reinterpret_cast<const IncomingSsnResetRequestParameter*>(parameter), responses);

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

			return responses;
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
				outgoingSsnResetRequestParameter->AddStream(streamId);
			}

			outgoingSsnResetRequestParameter->Consolidate();

			return reConfigChunk;
		}

		StreamResetHandler::ReqSeqNbrValidationResult StreamResetHandler::ValidateReqSeqNbr(
		  UnwrappedSequenceNumber<uint32_t> reqSeqNbr)
		{
			MS_TRACE();

			if (reqSeqNbr == this->lastProcessedReqSeqNbr)
			{
				return ReqSeqNbrValidationResult::RETRANSMISSION;
			}
			else if (reqSeqNbr != this->lastProcessedReqSeqNbr.GetNextValue())
			{
				// Too old, too new, from wrong Association, etc.
				MS_WARN_TAG(sctp, "bad reqSeqNbr");

				return ReqSeqNbrValidationResult::BADSEQUENCE_NUMBER;
			}
			else
			{
				return ReqSeqNbrValidationResult::VALID;
			}
		}

		void StreamResetHandler::HandleReceivedOutgoingSsnResetRequestParameter(
		  const OutgoingSsnResetRequestParameter* receivedOutgoingSsnResetRequestParameter,
		  std::vector<ReconfigurationResponseParameter>& responses)
		{
			MS_TRACE();

			// TODO: SCTP
		}

		void StreamResetHandler::HandleReceivedIncomingSsnResetRequestParameter(
		  const IncomingSsnResetRequestParameter* receivedIncomingSsnResetRequestParameter,
		  std::vector<ReconfigurationResponseParameter>& responses)
		{
			MS_TRACE();

			// TODO: SCTP
		}

		void StreamResetHandler::HandleReceivedReconfigurationResponseParameter(
		  const ReconfigurationResponseParameter* receivedReconfigurationResponseParameter)

		{
			MS_TRACE();

			// TODO: SCTP
		}

		void StreamResetHandler::OnReConfigTimer(uint64_t& baseTimeoutMs, bool& /*stop*/)
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
