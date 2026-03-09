#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Socket.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include <limits>      // std::numeric_limits()
#include <sstream>     // std::ostringstream
#include <type_traits> // std::is_same_v

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		thread_local static uint8_t PacketFactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];
		// @see https://tools.ietf.org/html/rfc9260#section-5.1
		constexpr uint32_t MinVerificationTag{ 1 };
		constexpr uint32_t MaxVerificationTag{ std::numeric_limits<uint32_t>::max() };
		// @see https://tools.ietf.org/html/rfc9260#section-3.3.2
		constexpr uint32_t MinInitialTsn{ 0 };
		constexpr uint32_t MaxInitialTsn{ std::numeric_limits<uint32_t>::max() };
		constexpr uint64_t MaxTieTag{ std::numeric_limits<uint64_t>::max() };

		/* Instance methods. */

		Socket::Socket(const SctpOptions& sctpOptions, SocketListener& listener)
		  : sctpOptions(sctpOptions),
		    // Our `listener` member is a `SocketDeferredListener` which takes
		    // `SocketListener` as constructor argument.
		    listener(listener),
		    // Create the `packetSender` member.
		    packetSender(*this, this->listener),
		    t1InitTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.t1InitTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ sctpOptions.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ sctpOptions.maxInitRetransmissions)),
		    t1CookieTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.t1CookieTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ sctpOptions.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ sctpOptions.maxInitRetransmissions)),
		    t2ShutdownTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ sctpOptions.t2ShutdownTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ sctpOptions.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ sctpOptions.maxRetransmissions))

		// TODO: Set RRSendQueue this->sendQueue.
		{
			MS_TRACE();
		}

		Socket::~Socket()
		{
			MS_TRACE();
		}

		void Socket::Dump(int indentation) const
		{
			MS_TRACE();

			const auto associationStateStringView =
			  Socket::AssociationStateToString(this->associationState);

			MS_DUMP_CLEAN(indentation, "<SCTP::Socket>");

			MS_DUMP_CLEAN(
			  indentation,
			  "  association state: %.*s",
			  static_cast<int>(associationStateStringView.size()),
			  associationStateStringView.data());

			const auto metrics = GetMetrics();

			if (metrics.has_value())
			{
				metrics->Dump(indentation + 1);
			}

			MS_DUMP_CLEAN(indentation, "</SCTP::Socket>");
		}

		Types::SocketState Socket::GetState() const
		{
			MS_TRACE();

			switch (this->associationState)
			{
				case AssociationState::CLOSED:
				{
					return Types::SocketState::CLOSED;
				}

				case AssociationState::COOKIE_WAIT:
				case AssociationState::COOKIE_ECHOED:
				{
					return Types::SocketState::CONNECTING;
				}

				case AssociationState::ESTABLISHED:
				{
					return Types::SocketState::CONNECTED;
				}

				case AssociationState::SHUTDOWN_PENDING:
				case AssociationState::SHUTDOWN_SENT:
				case AssociationState::SHUTDOWN_RECEIVED:
				case AssociationState::SHUTDOWN_ACK_SENT:
				{
					return Types::SocketState::SHUTTING_DOWN;
				}
			}
		}

		void Socket::Connect()
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			if (this->associationState == AssociationState::CLOSED)
			{
				this->preTcb.localVerificationTag =
				  Utils::Crypto::GetRandomUInt<uint32_t>(MinVerificationTag, MaxVerificationTag);
				this->preTcb.localInitialTsn =
				  Utils::Crypto::GetRandomUInt<uint32_t>(MinInitialTsn, MaxInitialTsn);

				SendInitChunk();

				this->t1InitTimer->Start();

				SetAssociationState(AssociationState::COOKIE_WAIT, "Connect() called");
			}
			if (this->associationState != AssociationState::CLOSED)
			{
				const auto associationStateStringView =
				  Socket::AssociationStateToString(this->associationState);

				MS_DEBUG_TAG(
				  sctp,
				  "cannot initiate the association since association state is not CLOSED but %.*s",
				  static_cast<int>(associationStateStringView.size()),
				  associationStateStringView.data());
			}

			AssertAssociationStateIsConsistent();
		}

		void Socket::Shutdown()
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "Upon receipt of the SHUTDOWN primitive from its upper layer, the
			// endpoint enters the SHUTDOWN-PENDING state and remains there until all
			// outstanding data has been acknowledged by its peer."
			if (this->tcb)
			{
				// TODO: Remove this check, as it just hides the problem that the Socket
				// can transition from ShutdownSent to ShutdownPending, or from
				// ShutdownAckSent to ShutdownPending, which is illegal.
				//
				// @see https://issues.webrtc.org/issues/42222897
				if (this->associationState != AssociationState::SHUTDOWN_SENT && this->associationState != AssociationState::SHUTDOWN_ACK_SENT)
				{
					SetAssociationState(AssociationState::SHUTDOWN_PENDING, "Shutdown() called");

					this->t1InitTimer->Stop();
					this->t1CookieTimer->Stop();

					MaySendShutdownOrShutdownAckChunk();
				}
			}
			// Connection closed before even starting to connect, or during the
			// initial connection phase. There is no outstanding data, so the Socket
			// can just be closed (stopping any connection timers, if any), as this
			// is the application's intention when calling Shutdown().
			else
			{
				InternalClose(Types::ErrorKind::NO_ERROR, "");
			}

			AssertAssociationStateIsConsistent();
		}

		void Socket::Close()
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			if (this->associationState != AssociationState::CLOSED)
			{
				if (this->tcb)
				{
					auto packet                 = this->tcb->CreatePacket();
					auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

					// NOTE: Don't set bit T in the ABORT chunk since TCB knows the
					// Verification Tag expected by the remote.

					auto* userInitiatedAbortErrorCause =
					  abortAssociationChunk->BuildErrorCauseInPlace<UserInitiatedAbortErrorCause>();

					userInitiatedAbortErrorCause->SetUpperLayerAbortReason("Close() called");

					userInitiatedAbortErrorCause->Consolidate();
					abortAssociationChunk->Consolidate();

					this->packetSender.SendPacket(packet.get());
				}

				InternalClose(Types::ErrorKind::NO_ERROR, "");
			}
			else
			{
				MS_DEBUG_TAG(sctp, "Close() called on a closed Socket");
			}

			AssertAssociationStateIsConsistent();
		}

		std::optional<SocketMetrics> Socket::GetMetrics() const
		{
			if (!this->tcb)
			{
				return std::nullopt;
			}

			// const size_t packetPayloadLength =
			//   this->sctpOptions.mtu - Packet::CommonHeaderLength - DataChunk::DataChunkHeaderLength;

			// TODO: Implement missing fields.
			SocketMetrics metrics{
				.txPacketsCount  = this->privateMetrics.txPacketsCount,
				.txMessagesCount = this->privateMetrics.txMessagesCount,
				.rxPacketsCount  = this->privateMetrics.rxPacketsCount,
				.rxMessagesCount = this->privateMetrics.rxMessagesCount,
				// .rtxPacketsCount = this->tcb->GetRetransmissionQueue().GetRtxPacketsCount(),
				// .rtxBytesCount   = this->tcb->GetRetransmissionQueue().GetRtxBytesCount(),
				// .cwndBytes       = this->tcb->GetCwnd(),
				// .srttMs          = this->tcb->GetCurrentSrttMs(),
				// .unackDataCount =
				//   this->tcb->GetRetransmissionQueue().GetUnackedItems() +
				//   (this->sendQueue.GetTotalBufferedAmount() + packetPayloadLength - 1) / packetPayloadLength,
				// .peerRwndBytes                = this->tcb->GetRetransmissionQueue().GetRwnd(),
				.peerImplementation           = this->privateMetrics.peerImplementation,
				.negotiatedMaxOutboundStreams = this->privateMetrics.negotiatedMaxOutboundStreams,
				.negotiatedMaxInboundStreams  = this->privateMetrics.negotiatedMaxInboundStreams,
				.usesPartialReliability       = this->privateMetrics.usesPartialReliability,
				.usesMessageInterleaving      = this->privateMetrics.usesMessageInterleaving,
				.usesReConfig                 = this->privateMetrics.usesReConfig,
				.usesZeroChecksum             = this->privateMetrics.usesZeroChecksum,

			};

			return metrics;
		}

		uint16_t Socket::GetStreamPriority(uint16_t streamId) const
		{
			MS_TRACE();

			// TODO: Implement it.
			// return this->sendQueue.GetStreamPriority(streamId);
		}

		void Socket::SetStreamPriority(uint16_t streamId, uint16_t priority)
		{
			MS_TRACE();

			// TODO: Implement it.
			// this->sendQueue.SetStreamPriority(streamId, priority);
		}

		void Socket::SetMaxSendMessageSize(size_t maxMessageSize)
		{
			MS_TRACE();

			this->sctpOptions.maxSendMessageSize = maxMessageSize;
		}

		size_t Socket::GetStreamBufferedAmount(uint16_t streamId) const
		{
			MS_TRACE();

			// TODO: Implement it.
			// return this->sendQueue.GetStreamBufferedAmount(streamId);
		}

		size_t Socket::GetStreamBufferedAmountLowThreshold(uint16_t streamId) const
		{
			MS_TRACE();

			// TODO: Implement it.
			// return this->sendQueue.GetStreamBufferedAmountLowThreshold(streamId);
		}

		void Socket::SetBufferedAmountLowThreshold(uint16_t streamId, size_t bytes)
		{
			MS_TRACE();

			// TODO: Implement it.
			// this->sendQueue.SetBufferedAmountLowThreshold(streamId, bytes);
		}

		Types::ResetStreamsStatus Socket::ResetStreams(std::span<const uint16_t> outboundStreamIds)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			if (!this->tcb)
			{
				this->listener.OnSocketError(
				  Types::ErrorKind::WRONG_SEQUENCE,
				  "cannot reset outbound streams as the socket is not connected");

				return Types::ResetStreamsStatus::NOT_CONNECTED;
			}

			// TODO: Implement it.
			// if (!this->tcb->GetCapabilities().reConfig)
			// {
			//   this->listener.OnSocketError(Types::ErrorKind::UNSUPPORTED_OPERATION,
			//                      "cannot reset outbound streams as the remote doesn't support it");

			//   return Types::ResetStreamsStatus::NOT_SUPPORTED;
			// }

			// TODO: Implement it.
			// this->tcb->GetStreamResetHandler().ResetStreams(outboundStreamIds);

			MaySendResetStreamsRequest();

			AssertAssociationStateIsConsistent();

			return Types::ResetStreamsStatus::PERFORMED;
		}

		Types::SendMessageStatus Socket::SendMessage(
		  Message message, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			Types::SendMessageStatus status = InternalSendMessage(message, sendMessageOptions);

			if (status != Types::SendMessageStatus::SUCCESS)
			{
				return status;
			}

			const uint64_t now = DepLibUV::GetTimeMs();

			this->privateMetrics.txMessagesCount++;

			// TODO: Implement it.
			// this->sendQueue.AddMessage(now, std::move(message), sendMessageOptions);

			if (this->tcb)
			{
				// TODO: Implement it.
				// this->tcb->SendBufferedPackets(now);
			}

			AssertAssociationStateIsConsistent();

			return Types::SendMessageStatus::SUCCESS;
		}

		std::vector<Types::SendMessageStatus> Socket::SendManyMessages(
		  std::span<Message> messages, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			const uint64_t now = DepLibUV::GetTimeMs();
			std::vector<Types::SendMessageStatus> statuses;

			statuses.reserve(messages.size());

			for (const auto& message : messages)
			{
				Types::SendMessageStatus status = InternalSendMessage(message, sendMessageOptions);

				statuses.push_back(status);

				if (status != Types::SendMessageStatus::SUCCESS)
				{
					continue;
				}

				this->privateMetrics.txMessagesCount++;

				// TODO: Implement it.
				// this->sendQueue.AddMessage(now, std::move(message), sendMessageOptions);
			}

			if (this->tcb)
			{
				// TODO: Implement it.
				// this->tcb->SendBufferedPackets(now);
			}

			AssertAssociationStateIsConsistent();

			return statuses;
		}

		void Socket::ReceivePacket(const Packet* receivedPacket)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			this->privateMetrics.rxPacketsCount++;

			if (!ValidateReceivedPacket(receivedPacket))
			{
				MS_WARN_TAG(sctp, "Packet verification failed, discarded");

				return;
			}

			MaySendShutdownOnPacketReceived(receivedPacket);

			for (auto it = receivedPacket->ChunksBegin(); it != receivedPacket->ChunksEnd(); ++it)
			{
				const auto* receivedChunk = *it;

				if (!ProcessReceivedChunk(receivedPacket, receivedChunk))
				{
					break;
				}
			}

			// TODO: Implement it.
			// if (this->tcb)
			// {
			//   this->tcb->GetDadaTracker().ObservePacketEnd();
			//   this->tcb->MaySendSack();
			// }

			AssertAssociationStateIsConsistent();
		}

		void Socket::InternalClose(Types::ErrorKind errorKind, const std::string_view& message)
		{
			MS_TRACE();

			if (this->associationState != AssociationState::CLOSED)
			{
				this->t1InitTimer->Stop();
				this->t1CookieTimer->Stop();
				this->t2ShutdownTimer->Stop();

				this->tcb = nullptr;
			}

			if (errorKind == Types::ErrorKind::NO_ERROR)
			{
				this->listener.OnSocketClosed();
			}
			else
			{
				this->listener.OnSocketAborted(errorKind, message);
			}

			SetAssociationState(AssociationState::CLOSED, message);
		}

		void Socket::SetAssociationState(AssociationState associationState, const std::string_view& message)
		{
			MS_TRACE();

			const auto associationStateStringView = Socket::AssociationStateToString(associationState);

			if (associationState == this->associationState)
			{
				MS_WARN_TAG(
				  sctp,
				  "association state is already %.*s (message: %.*s)",
				  static_cast<int>(associationStateStringView.size()),
				  associationStateStringView.data(),
				  static_cast<int>(message.size()),
				  message.data());

				return;
			}

			const auto previousAssociationStateStringView =
			  Socket::AssociationStateToString(this->associationState);

			MS_WARN_TAG(
			  sctp,
			  "association state changed from %.*s to %.*s (message: %.*s)",
			  static_cast<int>(previousAssociationStateStringView.size()),
			  previousAssociationStateStringView.data(),
			  static_cast<int>(associationStateStringView.size()),
			  associationStateStringView.data(),
			  static_cast<int>(message.size()),
			  message.data());

			this->associationState = associationState;
		}

		void Socket::AddCapabilitiesParametersToInitOrInitAckChunk(AnyInitChunk* chunk) const
		{
			MS_TRACE();

			auto* supportedExtensionsParameter =
			  chunk->BuildParameterInPlace<SupportedExtensionsParameter>();

			supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);

			if (this->sctpOptions.enablePartialReliability)
			{
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::FORWARD_TSN);

				auto* forwardTsnSupportedParameter =
				  chunk->BuildParameterInPlace<ForwardTsnSupportedParameter>();

				forwardTsnSupportedParameter->Consolidate();
			}

			if (this->sctpOptions.enableMessageInterleaving)
			{
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_FORWARD_TSN);
			}

			if (
			  this->sctpOptions.zeroChecksumAlternateErrorDetectionMethod !=
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE)
			{
				auto* zeroChecksumAcceptableParameter =
				  chunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

				zeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
				  this->sctpOptions.zeroChecksumAlternateErrorDetectionMethod);
				zeroChecksumAcceptableParameter->Consolidate();
			}

			supportedExtensionsParameter->Consolidate();
		}

		void Socket::CreateTransmissionControlBlock(
		  uint32_t localVerificationTag,
		  uint32_t remoteVerificationTag,
		  uint32_t localInitialTsn,
		  uint32_t remoteInitialTsn,
		  uint32_t remoteAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		{
			MS_TRACE();

			this->tcb = std::make_unique<TransmissionControlBlock>(
			  this->listener,
			  this->sctpOptions,
			  this->packetSender,
			  localVerificationTag,
			  remoteVerificationTag,
			  localInitialTsn,
			  remoteInitialTsn,
			  remoteAdvertisedReceiverWindowCredit,
			  tieTag,
			  negotiatedCapabilities);

			this->privateMetrics.negotiatedMaxOutboundStreams = negotiatedCapabilities.maxOutboundStreams;
			this->privateMetrics.negotiatedMaxInboundStreams  = negotiatedCapabilities.maxInboundStreams;
			this->privateMetrics.usesPartialReliability       = negotiatedCapabilities.partialReliability;
			this->privateMetrics.usesMessageInterleaving = negotiatedCapabilities.messageInterleaving;
			this->privateMetrics.usesReConfig            = negotiatedCapabilities.reConfig;
			this->privateMetrics.usesZeroChecksum        = negotiatedCapabilities.zeroChecksum;
		}

		std::unique_ptr<Packet> Socket::CreatePacket() const
		{
			MS_TRACE();

			return CreatePacketWithVerificationTag(0);
		}

		std::unique_ptr<Packet> Socket::CreatePacketWithVerificationTag(uint32_t verificationTag) const
		{
			MS_TRACE();

			auto packet =
			  std::unique_ptr<Packet>(Packet::Factory(PacketFactoryBuffer, sizeof(PacketFactoryBuffer)));

			packet->SetSourcePort(this->sctpOptions.sourcePort);
			packet->SetDestinationPort(this->sctpOptions.destinationPort);
			packet->SetVerificationTag(verificationTag);

			return packet;
		}

		void Socket::SendInitChunk()
		{
			MS_TRACE();

			auto packet = CreatePacket();

			// Insert an INIT Chunk in the Packet.
			auto* initChunk = packet->BuildChunkInPlace<InitChunk>();

			initChunk->SetInitiateTag(this->preTcb.localVerificationTag);
			initChunk->SetAdvertisedReceiverWindowCredit(this->sctpOptions.maxReceiverWindowBufferSize);
			initChunk->SetNumberOfOutboundStreams(this->sctpOptions.maxOutboundStreams);
			initChunk->SetNumberOfInboundStreams(this->sctpOptions.maxInboundStreams);
			initChunk->SetInitialTsn(this->preTcb.localInitialTsn);

			// Insert capabilities related Parameters in the INIT Chunk.
			AddCapabilitiesParametersToInitOrInitAckChunk(initChunk);

			initChunk->Consolidate();

			// https://datatracker.ietf.org/doc/html/rfc9653#section-5.2
			//
			// "When a sender sends a packet containing an INIT chunk, it MUST include
			// a correct CRC32c checksum in the packet containing the INIT chunk."
			this->packetSender.SendPacket(packet.get());
		}

		void Socket::SendShutdownChunk()
		{
			MS_TRACE();

			// TODO
		}

		void Socket::SendShutdownAckChunk()
		{
			MS_TRACE();

			AssertHasTcb();

			auto packet            = this->tcb->CreatePacket();
			auto* shutdownAckChunk = packet->BuildChunkInPlace<ShutdownAckChunk>();

			shutdownAckChunk->Consolidate();

			this->packetSender.SendPacket(packet.get());

			// TODO
			// this->t2ShutdownTimer->SetBaseTimeout(this->tcb->GetCurrentRto());
			this->t2ShutdownTimer->Start();
		}

		void Socket::MaySendShutdownOrShutdownAckChunk()
		{
			MS_TRACE();

			AssertHasTcb();

			// TODO: Implement it.
			// if (this->tcb->GetRetransmissionQueue().GetUnackedItems() != 0) {
			//   return;
			// }

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "Once all its outstanding data has been acknowledged, the endpoint
			// sends a SHUTDOWN chunk to its peer, including in the Cumulative TSN Ack
			// field the last sequential TSN it has received from the peer. It SHOULD
			// then start the T2-shutdown timer and enter the SHUTDOWN-SENT state."
			if (this->associationState == AssociationState::SHUTDOWN_PENDING)
			{
				SendShutdownChunk();

				// TODO: Implement it.
				// this->t2ShutdownTimer->SetBaseTimeoutMs(this->tcb->GetCurrentRtoMs());
				this->t2ShutdownTimer->Start();

				SetAssociationState(AssociationState::SHUTDOWN_SENT, "no more outstanding data");
			}
			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "If the receiver of the SHUTDOWN chunk has no more outstanding DATA
			// chunks, the SHUTDOWN chunk receiver MUST send a SHUTDOWN ACK chunk and
			// start a T2-shutdown timer of its own, entering the SHUTDOWN-ACK-SENT
			// state. If the timer expires, the endpoint MUST resend the SHUTDOWN ACK
			// chunk."
			else if (this->associationState == AssociationState::SHUTDOWN_RECEIVED)
			{
				SendShutdownAckChunk();

				SetAssociationState(AssociationState::SHUTDOWN_ACK_SENT, "no more outstanding data");
			}
		}

		void Socket::MaySendShutdownOnPacketReceived(const Packet* receivedPacket)
		{
			MS_TRACE();

			if (this->associationState != AssociationState::SHUTDOWN_SENT)
			{
				return;
			}

			AssertHasTcb();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "While in the SHUTDOWN-SENT state, the SHUTDOWN chunk sender MUST
			// immediately respond to each received packet containing one or more
			// DATA chunks with a SHUTDOWN chunk and restart the T2-shutdown timer."
			//
			// @remarks
			// - This also applies to I-DATA chunks.
			bool hasDataChunk = std::find_if(
			                      receivedPacket->ChunksBegin(),
			                      receivedPacket->ChunksEnd(),
			                      [](const Chunk* chunk)
			                      {
				                      return chunk->GetType() == Chunk::ChunkType::DATA ||
				                             chunk->GetType() == Chunk::ChunkType::I_DATA;
			                      }) != receivedPacket->ChunksEnd();

			if (hasDataChunk)
			{
				SendShutdownChunk();

				// TODO: Implement it.
				// this->t2ShutdownTimer->SetBaseTimeoutMs(this->tcb->GetCurrentRtoMs());
				this->t2ShutdownTimer->Start();
			}
		}

		void Socket::MaySendResetStreamsRequest()
		{
			MS_TRACE();

			AssertHasTcb();

			// TODO: I don't like this. I don't want to use Packet::AddChunk() (which
			// clones the given Chunk). I want to use Packet::BuildChunkInPlace() so
			// we need that `tcb->GetStreamResetHandler().MakeStreamResetRequest()`
			// doesn't return a `ReConfigChunk` but something different such as the
			// Re-configuration Request Parameter(s) (OutgoingSSNResetRequestParameter):
			// https://datatracker.ietf.org/doc/html/rfc6525#section-8.2
			// Mmmm, but not even that because we also want to use
			// ReConfigChunk::BuildParameterInPlace() instead of AddParameter() for
			// same reasons... Ok, let's see.
			// NOTE: What about if we do some std::move() somewhere?

			// const auto* reConfigChunk =
			//     this->tcb->GetStreamResetHandler().MakeStreamResetRequest();
			// const auto* outgoingSSNResetRequestParameter =
			//   this->tcb->GetStreamResetHandler().MakeOutgoingSSNResetRequestParameter();

			// if (!outgoingSSNResetRequestParameter)
			// {
			// 	return;
			// }

			// auto packet         = this->tcb->CreatePacket();
			// auto* reConfigChunk = packet->BuildChunkInPlace<ReConfigChunk>();

			// reConfigChunk->AddParameter(outgoingSSNResetRequestParameter);

			// delete outgoingSSNResetRequestParameter;

			// reConfigChunk->Consolidate();

			// this->packetSender.SendPacket(packet.get());
		}

		void Socket::MayDeliverMessages()
		{
			MS_TRACE();

			AssertHasTcb();

			// TODO: Implement it.
			// while (std::optional<Message> message = this->tcb->GetReassemblyQueue().GetNextMessage())
			// {
			// 	this->privateMetrics.rxMessagesCount++;
			// 	this->listener.OnSocketMessageReceived(*std::move(message));
			// }
		}

		Types::SendMessageStatus Socket::InternalSendMessage(
		  const Message& message, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			const auto lifecycleId = sendMessageOptions.lifecycleId;

			if (message.GetPayloadLength() == 0)
			{
				if (lifecycleId.has_value())
				{
					this->listener.OnSocketLifecycleMessageEnd(lifecycleId.value());
				}

				this->listener.OnSocketError(
				  Types::ErrorKind::PROTOCOL_VIOLATION, "cannot send empty message");

				return Types::SendMessageStatus::ERROR_MESSAGE_EMPTY;
			}
			else if (message.GetPayloadLength() > this->sctpOptions.maxSendMessageSize)
			{
				if (lifecycleId.has_value())
				{
					this->listener.OnSocketLifecycleMessageEnd(lifecycleId.value());
				}

				this->listener.OnSocketError(
				  Types::ErrorKind::PROTOCOL_VIOLATION, "cannot send too large message");

				return Types::SendMessageStatus::ERROR_MESSAGE_TOO_LARGE;
			}
			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "An endpoint SHOULD reject any new data request from its upper layer
			// if it is in the SHUTDOWN-PENDING, SHUTDOWN-SENT, SHUTDOWN-RECEIVED, or
			// SHUTDOWN-ACK-SENT state."
			else if (
			  this->associationState == AssociationState::SHUTDOWN_PENDING ||
			  this->associationState == AssociationState::SHUTDOWN_SENT ||
			  this->associationState == AssociationState::SHUTDOWN_RECEIVED ||
			  this->associationState == AssociationState::SHUTDOWN_ACK_SENT)
			{
				if (lifecycleId.has_value())
				{
					this->listener.OnSocketLifecycleMessageEnd(lifecycleId.value());
				}

				this->listener.OnSocketError(
				  Types::ErrorKind::WRONG_SEQUENCE, "cannot send message as the socket is shutting down");

				return Types::SendMessageStatus::ERROR_SHUTTING_DOWN;
			}
			// TODO: Implement it.
			// else if (
			//   this->sendQueue.GetTotalBufferedAmount() >= this->sctpOptions.maxSendBufferSize ||
			//   this->sendQueue.GetStreamBufferedAmount(message.GetStreamId()) >=
			//     this->sctpOptions.perStreamSendQueueLimit)
			// {
			// 	if (lifecycleId.has_value())
			// 	{
			// 		this->listener.OnSocketLifecycleMessageEnd(lifecycleId.value());
			// 	}

			// 	this->listener.OnSocketError(
			// 	  Types::ErrorKind::RESOURCE_EXHAUSTION, "cannot send message as the send queue is full");

			// 	return Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION;
			// }

			return Types::SendMessageStatus::SUCCESS;
		}

		bool Socket::ValidateReceivedPacket(const Packet* receivedPacket)
		{
			MS_TRACE();

			uint32_t localVerificationTag = this->tcb ? this->tcb->GetLocalVerificationTag() : 0;

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
			//
			// "When an endpoint receives an SCTP packet with the Verification Tag
			// set to 0, it SHOULD verify that the packet contains only an INIT
			// chunk. Otherwise, the receiver MUST silently discard the packet."
			if (receivedPacket->GetVerificationTag() == 0)
			{
				if (receivedPacket->GetChunksCount() == 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::INIT)
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "Packet with Verification Tag 0 must have a single Chunk and it must be an INIT Chunk, packet discarded");

					this->listener.OnSocketError(
					  Types::ErrorKind::PARSE_FAILED,
					  "packet with Verification Tag 0 must have a single chunk and it must be an INIT chunk");

					return false;
				}
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
			//
			// "The receiver of an ABORT chunk MUST accept the packet if the
			// Verification Tag field of the packet matches its own tag and the T bit
			// is not set OR if it is set to its Peer's Tag and the T bit is set in
			// the Chunk Flags. Otherwise, the receiver MUST silently discard the
			// packet and take no further action."
			if (receivedPacket->GetChunksCount() == 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::ABORT)
			{
				const auto* abortAssociationChunk =
				  static_cast<const AbortAssociationChunk*>(receivedPacket->GetChunkAt(0));

				// We cannot verify the Verification Tag so assume it's okey.
				if (abortAssociationChunk->GetT() && !this->tcb)
				{
					return true;
				}
				else if (
				  (!abortAssociationChunk->GetT() &&
				   receivedPacket->GetVerificationTag() == localVerificationTag) ||
				  (abortAssociationChunk->GetT() &&
				   receivedPacket->GetVerificationTag() == this->tcb->GetRemoteVerificationTag()))
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "ABORT Chunk Verification Tag %" PRIu32 " is wrong, packet discarded",
					  receivedPacket->GetVerificationTag());

					this->listener.OnSocketError(
					  Types::ErrorKind::PARSE_FAILED, "packet with ABORT chunk has invalid Verification Tag");

					return false;
				}
			}

			if (receivedPacket->GetChunksCount() >= 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::INIT_ACK)
			{
				if (receivedPacket->GetVerificationTag() == this->preTcb.localVerificationTag)
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "INIT_ACK Chunk Verification Tag %" PRIu32 " (should be %" PRIu32 ")",
					  receivedPacket->GetVerificationTag(),
					  this->preTcb.localVerificationTag);

					this->listener.OnSocketError(
					  Types::ErrorKind::PARSE_FAILED,
					  "packet with INIT_ACK chunk has invalid Verification Tag");

					return false;
				}
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.4
			//
			// This is handled in ProcessReceivedCookieEchoChunk().
			if (receivedPacket->GetChunksCount() >= 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::COOKIE_ECHO)
			{
				return true;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
			//
			// "The receiver of a SHUTDOWN COMPLETE shall accept the packet if the
			// Verification Tag field of the packet matches its own tag and the T bit is
			// not set OR if it is set to its peer's tag and the T bit is set in the
			// Chunk Flags.  Otherwise, the receiver MUST silently discard the packet
			// and take no further action."
			if (receivedPacket->GetChunksCount() == 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::SHUTDOWN_COMPLETE)
			{
				const auto* shutdownCompleteChunk =
				  static_cast<const ShutdownCompleteChunk*>(receivedPacket->GetChunkAt(0));

				// We cannot verify the Verification Tag so assume it's okey.
				if (shutdownCompleteChunk->GetT() && !this->tcb)
				{
					return true;
				}
				else if (
				  (!shutdownCompleteChunk->GetT() &&
				   receivedPacket->GetVerificationTag() == localVerificationTag) ||
				  (shutdownCompleteChunk->GetT() &&
				   receivedPacket->GetVerificationTag() == this->tcb->GetRemoteVerificationTag()))
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "SHUTDOWN_COMPLETE Chunk Verification Tag %" PRIu32 " is wrong, packet discarded",
					  receivedPacket->GetVerificationTag());

					this->listener.OnSocketError(
					  Types::ErrorKind::PARSE_FAILED,
					  "packet with SHUTDOWN_COMPLETE chunk has invalid Verification Tag");

					return false;
				}
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-8.5
			//
			// "When receiving an SCTP packet, the endpoint MUST ensure that the
			// value in the Verification Tag field of the received SCTP packet
			// matches its own tag. If the received Verification Tag value does not
			// match the receiver's own tag value, the receiver MUST silently discard
			// the packet and MUST NOT process it any further, except for those cases
			// listed in Section 8.5.1 below."
			if (receivedPacket->GetVerificationTag() == localVerificationTag)
			{
				return true;
			}
			else
			{
				MS_WARN_TAG(
				  sctp,
				  "invalid Verification Tag %" PRIu32 " (should be %" PRIu32 ")",
				  receivedPacket->GetVerificationTag(),
				  localVerificationTag);

				this->listener.OnSocketError(
				  Types::ErrorKind::PARSE_FAILED, "packet has invalid Verification Tag");

				return false;
			}
		}

		bool Socket::ProcessReceivedChunk(const Packet* receivedPacket, const Chunk* receivedChunk)
		{
			MS_TRACE();

			switch (receivedChunk->GetType())
			{
				case Chunk::ChunkType::INIT:
				{
					ProcessReceivedInitChunk(receivedPacket, static_cast<const InitChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::INIT_ACK:
				{
					ProcessReceivedInitAckChunk(receivedPacket, static_cast<const InitAckChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::COOKIE_ECHO:
				{
					ProcessReceivedCookieEchoChunk(
					  receivedPacket, static_cast<const CookieEchoChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::COOKIE_ACK:
				{
					ProcessReceivedCookieAckChunk(
					  receivedPacket, static_cast<const CookieAckChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::SHUTDOWN:
				{
					ProcessReceivedShutdownChunk(
					  receivedPacket, static_cast<const ShutdownChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::SHUTDOWN_ACK:
				{
					ProcessReceivedShutdownAckChunk(
					  receivedPacket, static_cast<const ShutdownAckChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::SHUTDOWN_COMPLETE:
				{
					ProcessReceivedShutdownCompleteChunk(
					  receivedPacket, static_cast<const ShutdownCompleteChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::OPERATION_ERROR:
				{
					ProcessReceivedOperationErrorChunk(
					  receivedPacket, static_cast<const OperationErrorChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::ABORT:
				{
					ProcessReceivedAbortAssociationChunk(
					  receivedPacket, static_cast<const AbortAssociationChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::HEARTBEAT_REQUEST:
				{
					ProcessReceivedHeartbeatRequestChunk(
					  receivedPacket, static_cast<const HeartbeatRequestChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::HEARTBEAT_ACK:
				{
					ProcessReceivedHeartbeatAckChunk(
					  receivedPacket, static_cast<const HeartbeatAckChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::RE_CONFIG:
				{
					ProcessReceivedReConfigChunk(
					  receivedPacket, static_cast<const ReConfigChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::FORWARD_TSN:
				{
					ProcessReceivedForwardTsnChunk(
					  receivedPacket, static_cast<const ForwardTsnChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::I_FORWARD_TSN:
				{
					ProcessReceivedIForwardTsnChunk(
					  receivedPacket, static_cast<const IForwardTsnChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::DATA:
				{
					ProcessReceivedDataChunk(receivedPacket, static_cast<const DataChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::I_DATA:
				{
					ProcessReceivedIDataChunk(receivedPacket, static_cast<const IDataChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::SACK:
				{
					ProcessReceivedSackChunk(receivedPacket, static_cast<const SackChunk*>(receivedChunk));

					break;
				}

				default:
				{
					return ProcessReceivedUnknownChunk(
					  receivedPacket, static_cast<const UnknownChunk*>(receivedChunk));
				}
			}

			return true;
		}

		void Socket::ProcessReceivedInitChunk(
		  const Packet* /*receivedPacket*/, const InitChunk* receivedInitChunk)
		{
			MS_TRACE();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-3.3.2
			//
			// "If the value of the Initiate Tag in a received INIT chunk is found to
			// be 0, the receiver MUST silently discard the packet."
			if (receivedInitChunk->GetInitiateTag() == 0)
			{
				MS_WARN_TAG(sctp, "invalid value 0 in Initiate Tagin received INIT Chunk, discarded");

				return;
			}
			// https://datatracker.ietf.org/doc/html/rfc9260#section-3.3.2
			//
			// "A receiver of an INIT chunk with the OS value set to 0 MUST discard
			// the packet, SHOULD send a packet in response containing an ABORT chunk
			// and using the Initiate Tag as the Verification Tag."
			//
			// "A receiver of an INIT chunk with the MIS value set to 0 MUST discard
			// the packet, SHOULD send a packet in response containing an ABORT chunk
			// and using the Initiate Tag as the Verification Tag."
			else if (
			  receivedInitChunk->GetNumberOfOutboundStreams() == 0 or
			  receivedInitChunk->GetNumberOfInboundStreams() == 0)
			{
				MS_WARN_TAG(
				  sctp,
				  "invalidNumber of Outbound Streams or Number of Inbound Streams in received INIT Chunk, aborting association");

				auto packet                 = CreatePacketWithVerificationTag(0);
				auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: We are not setting the Verification Tag expected by the peer
				// so must set be T to 1.
				abortAssociationChunk->SetT(true);

				auto* protocolViolationErrorCause =
				  abortAssociationChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "invalid value 0 in Number of Outbound Streams or Number of Inbound Streams in received INIT chunk");

				protocolViolationErrorCause->Consolidate();
				abortAssociationChunk->Consolidate();

				this->packetSender.SendPacket(packet.get());

				InternalClose(Types::ErrorKind::PROTOCOL_VIOLATION, "received invalid INIT chunk");

				return;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "If an endpoint is in the SHUTDOWN-ACK-SENT state and receives an INIT
			// chunk (e.g., if the SHUTDOWN COMPLETE chunk was lost) with source and
			// destination transport addresses (either in the IP addresses or in the
			// INIT chunk) that belong to this association, it SHOULD discard the
			// INIT chunk and retransmit the SHUTDOWN ACK chunk."
			if (this->associationState == AssociationState::SHUTDOWN_ACK_SENT)
			{
				MS_DEBUG_TAG(
				  sctp, "INIT Chunk received in SHUTDOWN_ACK_SENT state, retransmitting SHUTDOWN_ACK Chunk");

				SendShutdownAckChunk();

				return;
			}

			uint64_t tieTag{ 0 };
			uint32_t localVerificationTag;
			uint32_t localInitialTsn;

			switch (this->associationState)
			{
				case AssociationState::CLOSED:
				{
					MS_DEBUG_TAG(sctp, "INIT Chunk received in CLOSED state (normal scenario)");

					localVerificationTag =
					  Utils::Crypto::GetRandomUInt<uint32_t>(MinVerificationTag, MaxVerificationTag);
					localInitialTsn = Utils::Crypto::GetRandomUInt<uint32_t>(MinInitialTsn, MaxInitialTsn);

					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.1
				//
				// "This usually indicates an initialization collision, i.e., each
				// endpoint is attempting, at about the same time, to establish an
				// association with the other endpoint. Upon receipt of an INIT chunk
				// in the COOKIE-WAIT state, an endpoint MUST respond with an INIT ACK
				// chunk using the same parameters it sent in its original INIT chunk
				// (including its Initiate Tag, unchanged)."
				case AssociationState::COOKIE_WAIT:
				case AssociationState::COOKIE_ECHOED:
				{
					MS_DEBUG_TAG(sctp, "INIT Chunk received after sending INIT Chunk (collision, no problem)");

					localVerificationTag = this->preTcb.localVerificationTag;
					localInitialTsn      = this->preTcb.localInitialTsn;

					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.2
				//
				// "The outbound SCTP packet containing this INIT ACK chunk MUST carry
				// a Verification Tag value equal to the Initiate Tag found in the
				// unexpected INIT chunk. And the INIT ACK chunk MUST contain a new
				// Initiate Tag (randomly generated; see Section 5.3.1). Other
				// parameters for the endpoint SHOULD be copied from the existing
				// parameters of the association (e.g., number of outbound streams)
				// into the INIT ACK chunk and cookie."
				default:
				{
					AssertHasTcb();

					MS_DEBUG_TAG(sctp, "INIT Chunk received (probably peer restarted)");

					localVerificationTag =
					  Utils::Crypto::GetRandomUInt<uint32_t>(MinVerificationTag, MaxVerificationTag);

					localInitialTsn = Utils::Crypto::GetRandomUInt<uint32_t>(MinInitialTsn, MaxInitialTsn);
					tieTag          = this->tcb->GetTieTag();
				}
			}

			MS_DEBUG_TAG(
			  sctp,
			  "initiating association [localVerificationTag:%" PRIu32 ", localInitialTsn:%" PRIu32
			  ", remoteVerificationTag:%" PRIu32 ", remoteInitialTsn:%" PRIu32 "]",
			  localVerificationTag,
			  localInitialTsn,
			  receivedInitChunk->GetInitiateTag(),
			  receivedInitChunk->GetInitialTsn());

			/* Send a Packet with an INIT_ACK Chunk. */

			auto packet = CreatePacketWithVerificationTag(receivedInitChunk->GetInitiateTag());

			// Insert an INIT_ACK Chunk in the Packet.
			auto* initAckChunk = packet->BuildChunkInPlace<InitAckChunk>();

			initAckChunk->SetInitiateTag(localVerificationTag);
			initAckChunk->SetAdvertisedReceiverWindowCredit(this->sctpOptions.maxReceiverWindowBufferSize);
			initAckChunk->SetNumberOfOutboundStreams(this->sctpOptions.maxOutboundStreams);
			initAckChunk->SetNumberOfInboundStreams(this->sctpOptions.maxInboundStreams);
			initAckChunk->SetInitialTsn(localInitialTsn);

			// Insert a StateCookieParameter in the INIT_ACK Chunk.
			auto* stateCookieParameter = initAckChunk->BuildParameterInPlace<StateCookieParameter>();

			const auto negotiatedCapabilities =
			  NegotiatedCapabilities::Factory(this->sctpOptions, receivedInitChunk);

			// Write the StateCookie in place in the Parameter.
			stateCookieParameter->WriteStateCookieInPlace(
			  localVerificationTag,
			  receivedInitChunk->GetInitiateTag(),
			  localInitialTsn,
			  receivedInitChunk->GetInitialTsn(),
			  receivedInitChunk->GetAdvertisedReceiverWindowCredit(),
			  tieTag,
			  negotiatedCapabilities);

			stateCookieParameter->Consolidate();

			// Insert capabilities related Parameters in the INIT_ACK Chunk.
			AddCapabilitiesParametersToInitOrInitAckChunk(initAckChunk);

			initAckChunk->Consolidate();

			// If the peer has signaled that it supports Zero Checksum, INIT-ACK can
			// then have its checksum as zero.
			this->packetSender.SendPacket(
			  packet.get(), /*writeChecksum*/ !negotiatedCapabilities.zeroChecksum);
		}

		void Socket::ProcessReceivedInitAckChunk(
		  const Packet* /*receivedPacket*/, const InitAckChunk* receivedInitAckChunk)
		{
			MS_TRACE();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.3
			//
			// "If an INIT ACK chunk is received by an endpoint in any state other
			// than the COOKIE-WAIT or CLOSED state, the endpoint SHOULD discard the
			// INIT ACK chunk."
			if (this->associationState != AssociationState::COOKIE_WAIT)
			{
				MS_DEBUG_TAG(sctp, "ignoring received INIT_ACK Chunk when not in COOKIE_WAIT state");

				return;
			}

			const auto* stateCookieParameter =
			  receivedInitAckChunk->GetFirstParameterOfType<StateCookieParameter>();

			if (!stateCookieParameter || !stateCookieParameter->GetCookie())
			{
				MS_WARN_TAG(
				  sctp, "ignoring received INIT_ACK Chunk without StateCookieParameter or without Cookie");

				auto packet = CreatePacketWithVerificationTag(this->preTcb.localVerificationTag);
				auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: We are not setting the Verification Tag expected by the peer
				// so must set be T to 1.
				abortAssociationChunk->SetT(true);

				auto* protocolViolationErrorCause =
				  abortAssociationChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "INIT_ACK without State Cookie Parameter or without Cookie");

				protocolViolationErrorCause->Consolidate();
				abortAssociationChunk->Consolidate();

				this->packetSender.SendPacket(packet.get());

				InternalClose(
				  Types::ErrorKind::PROTOCOL_VIOLATION, "received INIT_ACK chunk doesn't contain a Cookie");

				return;
			}

			this->privateMetrics.peerImplementation = StateCookie::DetermineSctpImplementation(
			  stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength());

			this->t1InitTimer->Stop();

			const auto negotiatedCapabilities =
			  NegotiatedCapabilities::Factory(this->sctpOptions, receivedInitAckChunk);

			// If the connection is re-established (peer restarted, but re-used old
			// connection), make sure that all message identifiers are reset and any
			// partly sent message is re-sent in full. The same is true when the Socket
			// is closed and later re-opened, which never happens in WebRTC, but is a
			// valid operation on the SCTP level.
			// TODO: Implement it.
			// this->sendQueue.Reset();

			CreateTransmissionControlBlock(
			  this->preTcb.localVerificationTag,
			  receivedInitAckChunk->GetInitiateTag(),
			  this->preTcb.localInitialTsn,
			  receivedInitAckChunk->GetInitialTsn(),
			  receivedInitAckChunk->GetAdvertisedReceiverWindowCredit(),
			  /*tieTag*/ Utils::Crypto::GetRandomUInt<uint64_t>(0, MaxTieTag),
			  negotiatedCapabilities);

			SetAssociationState(AssociationState::COOKIE_ECHOED, "INIT_ACK received");

			// The connection isn't fully established just yet.
			// TODO: Implement it.
			// this->tcb->SetCookieEchoChunk(CookieEchoChunk(cookie->data()));
			// this->tcb->SendBufferedPackets(callbacks_.Now());

			this->t1CookieTimer->Start();
		}

		void Socket::ProcessReceivedCookieEchoChunk(
		  const Packet* receivedPacket, const CookieEchoChunk* receivedCookieEchoChunk)
		{
			MS_TRACE();

			const auto* stateCookieParameter =
			  receivedCookieEchoChunk->GetFirstParameterOfType<StateCookieParameter>();

			if (!stateCookieParameter || !stateCookieParameter->GetCookie())
			{
				MS_WARN_TAG(
				  sctp, "ignoring received COOKIE_ECHO Chunk without StateCookieParameter or without Cookie");

				this->listener.OnSocketError(
				  Types::ErrorKind::PARSE_FAILED,
				  "received COOKIE_ECHO Chunk without StateCookieParameter or without Cookie");

				return;
			}

			std::unique_ptr<StateCookie> cookie{ StateCookie::Parse(
				stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength()) };

			if (!cookie)
			{
				MS_WARN_TAG(sctp, "failed to parse Cookie in received COOKIE_ECHO Chunk");

				this->listener.OnSocketError(
				  Types::ErrorKind::PARSE_FAILED, "received COOKIE_ECHO Chunk with invalid Cookie");

				return;
			}

			if (this->tcb)
			{
				if (!ProcessReceivedCookieEchoChunkWithTcb(receivedPacket, cookie.get()))
				{
					return;
				}
			}
			else
			{
				if (receivedPacket->GetVerificationTag() != cookie->GetLocalVerificationTag())
				{
					MS_WARN_TAG(sctp, "received COOKIE_ECHO Chunk with invalid Verification Tag");

					this->listener.OnSocketError(
					  Types::ErrorKind::PARSE_FAILED,
					  "received COOKIE_ECHO Chunk with invalid Verification Tag");

					return;
				}
			}

			this->t1InitTimer->Stop();
			this->t1CookieTimer->Stop();

			if (this->associationState != AssociationState::ESTABLISHED)
			{
				if (this->tcb)
				{
					// TODO: Implement it.
					// this->tcb->ClearCookieEchoChunk();
				}

				SetAssociationState(AssociationState::ESTABLISHED, "COOKIE_ECHO received");

				this->listener.OnSocketConnected();
			}

			if (!this->tcb)
			{
				// If the connection is re-established (peer restarted, but re-used old
				// connection), make sure that all message identifiers are reset and any
				// partly sent message is re-sent in full. The same is true when the
				// Socket is closed and later re-opened, which never happens in WebRTC,
				// but is a valid operation on the SCTP level.
				// TODO: Implement it.
				// this->sendQqueue.Reset();

				CreateTransmissionControlBlock(
				  cookie->GetLocalVerificationTag(),
				  cookie->GetRemoteVerificationTag(),
				  cookie->GetLocalInitialTsn(),
				  cookie->GetRemoteInitialTsn(),
				  cookie->GetRemoteAdvertisedReceiverWindowCredit(),
				  /*tieTag*/ Utils::Crypto::GetRandomUInt<uint64_t>(0, MaxTieTag),
				  cookie->GetNegotiatedCapabilities());
			}

			auto packet          = this->tcb->CreatePacket();
			auto* cookieAckChunk = packet->BuildChunkInPlace<CookieAckChunk>();

			cookieAckChunk->Consolidate();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-5.1
			//
			// "A COOKIE ACK chunk MAY be bundled with any pending DATA chunks (and/or
			// SACK chunks), but the COOKIE ACK chunk MUST be the first chunk in the
			// packet."
			// TODO: Implement it. Note that we pass Packet as argument!
			// this->tcb->SendBufferedPackets(packet.get(), callbacks_.Now());
		}

		bool Socket::ProcessReceivedCookieEchoChunkWithTcb(
		  const Packet* receivedPacket, const StateCookie* cookie)
		{
			MS_TRACE();

			MS_DEBUG_DEV("handling COOKIE_ECHO with TCB");

			AssertHasTcb();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.4
			//
			// "Handle a COOKIE ECHO Chunk When a TCB Exists"
			//
			// "A) In this case, the peer might have restarted."
			if (
			  receivedPacket->GetVerificationTag() != this->tcb->GetLocalVerificationTag() &&
			  cookie->GetRemoteVerificationTag() != this->tcb->GetRemoteVerificationTag() &&
			  cookie->GetTieTag() == this->tcb->GetTieTag())
			{
				// "If the endpoint is in the SHUTDOWN-ACK-SENT state and recognizes
				// that the peer has restarted (Action A), it MUST NOT set up a new
				// association but instead resend the SHUTDOWN ACK chunk and send an
				// ERROR chunk with a "Cookie Received While Shutting Down" error cause
				// to its peer."
				if (this->associationState == AssociationState::SHUTDOWN_ACK_SENT)
				{
					auto packet = CreatePacketWithVerificationTag(cookie->GetRemoteVerificationTag());
					auto* shutdownAckChunk = packet->BuildChunkInPlace<ShutdownAckChunk>();

					shutdownAckChunk->Consolidate();

					auto* operationErrorChunk = packet->BuildChunkInPlace<OperationErrorChunk>();
					auto* cookieReceivedWhileShuttingDownErrorCause =
					  operationErrorChunk->BuildErrorCauseInPlace<CookieReceivedWhileShuttingDownErrorCause>();

					cookieReceivedWhileShuttingDownErrorCause->Consolidate();
					operationErrorChunk->Consolidate();

					this->packetSender.SendPacket(packet.get());

					this->listener.OnSocketError(
					  Types::ErrorKind::WRONG_SEQUENCE, "received COOKIE_ECHO while shutting down");

					return false;
				}

				MS_DEBUG_DEV("received COOKIE_ECHO indicating a restarted peer");

				this->tcb = nullptr;
				this->listener.OnSocketConnectionRestarted();
			}
			// "B) In this case, both sides might be attempting to start an association
			// at about the same time, but the peer endpoint sent its INIT chunk after
			// responding to the local endpoint's INIT chunk."
			else if (
			  receivedPacket->GetVerificationTag() == this->tcb->GetLocalVerificationTag() &&
			  cookie->GetRemoteVerificationTag() != this->tcb->GetRemoteVerificationTag())
			{
				// TODO: Handle the case in which remote Verification Tag is 0?

				MS_DEBUG_DEV("received COOKIE_ECHO indicating simultaneous connections");

				this->tcb = nullptr;
			}
			// "C) In this case, the local endpoint's cookie has arrived late. Before
			// it arrived, the local endpoint sent an INIT chunk and received an INIT
			// ACK chunk and finally sent a COOKIE ECHO chunk with the peer's same tag
			// but a new tag of its own. The cookie SHOULD be silently discarded. The
			// endpoint SHOULD NOT change states and SHOULD leave any timers running."
			else if (
			  receivedPacket->GetVerificationTag() != this->tcb->GetLocalVerificationTag() &&
			  cookie->GetRemoteVerificationTag() == this->tcb->GetRemoteVerificationTag() &&
			  cookie->GetTieTag() == this->tcb->GetTieTag())
			{
				MS_DEBUG_DEV("received COOKIE_ECHO indicating a late COOKIE_ECHO, discarding");

				return false;
			}
			// "D) When both local and remote tags match, the endpoint SHOULD enter
			// the ESTABLISHED state if it is in the COOKIE_ECHOED state. It SHOULD
			// stop any T1-cookie timer that is running and send a COOKIE ACK chunk."
			else if (
			  receivedPacket->GetVerificationTag() == this->tcb->GetLocalVerificationTag() &&
			  cookie->GetRemoteVerificationTag() == this->tcb->GetRemoteVerificationTag())
			{
				MS_DEBUG_DEV(
				  "received duplicate COOKIE_ECHO, probably because of peer not receiving COOKIE_ACK and retransmitting COOKIE_ECHO");
			}

			return true;
		}

		void Socket::ProcessReceivedCookieAckChunk(
		  const Packet* /*receivedPacket*/, const CookieAckChunk* /*receivedCookieAckChunk*/)
		{
			MS_TRACE();

			// https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.5
			//
			// "At any state other than COOKIE_ECHOED, an endpoint SHOULD silently
			// discard a received COOKIE ACK chunk."
			if (this->associationState != AssociationState::COOKIE_ECHOED)
			{
				MS_DEBUG_DEV("received COOKIE_ACK not in COOKIE_ECHOED state, discarding");

				return;
			}

			AssertHasTcb();

			this->t1CookieTimer->Stop();

			// TODO: Implement this.
			// this->tcb->ClearCookieEchoChunk();

			SetAssociationState(AssociationState::ESTABLISHED, "COOKIE_ACK received");

			// TODO: Implement this.
			// this->tcb->SendBufferedPackets(callbacks_.Now());

			this->listener.OnSocketConnected();
		}

		void Socket::ProcessReceivedShutdownChunk(
		  const Packet* /*receivedPacket*/, const ShutdownChunk* /*receivedShutdownChunk*/)
		{
			MS_TRACE();

			switch (this->associationState)
			{
				case AssociationState::CLOSED:
				{
					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
				//
				// "If a SHUTDOWN chunk is received in the COOKIE-WAIT or COOKIE ECHOED
				// state, the SHUTDOWN chunk SHOULD be silently discarded."
				case AssociationState::COOKIE_WAIT:
				case AssociationState::COOKIE_ECHOED:
				{
					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
				//
				// "If an endpoint is in the SHUTDOWN-SENT state and receives a SHUTDOWN
				// chunk from its peer, the endpoint SHOULD respond immediately with a
				// SHUTDOWN ACK chunk to its peer and move into the SHUTDOWN-ACK-SENT
				// state, restarting its T2-shutdown timer.
				case AssociationState::SHUTDOWN_SENT:
				{
					SendShutdownAckChunk();
					SetAssociationState(AssociationState::SHUTDOWN_ACK_SENT, "SHUTDOWN received");

					break;
				}

				// TODO: This case block should be removed and handled by the `default`
				// case block.
				//
				// @see https://issues.webrtc.org/issues/42222897
				case AssociationState::SHUTDOWN_ACK_SENT:
				{
					break;
				}

				case AssociationState::SHUTDOWN_RECEIVED:
				{
					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
				//
				// "Upon reception of the SHUTDOWN chunk, the peer endpoint does the
				// following:
				// - enter the SHUTDOWN-RECEIVED state,
				// - stop accepting new data from its SCTP user, and
				// - verify, by checking the Cumulative TSN Ack field of the chunk,
				//   that all its outstanding DATA chunks have been received by the
				//   SHUTDOWN chunk sender."
				default:
				{
					MS_DEBUG_DEV("received SHUTDOWN, shutting down the Socket");

					SetAssociationState(AssociationState::SHUTDOWN_RECEIVED, "SHUTDOWN received");
					MaySendShutdownOrShutdownAckChunk();
				}
			}
		}

		void Socket::ProcessReceivedShutdownAckChunk(
		  const Packet* receivedPacket, const ShutdownAckChunk* receivedShutdownAckChunk)
		{
			MS_TRACE();

			switch (this->associationState)
			{
				// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
				//
				// "Upon the receipt of the SHUTDOWN ACK chunk, the sender of the
				// SHUTDOWN chunk MUST stop the T2-shutdown timer, send a SHUTDOWN
				// COMPLETE chunk to its peer, and remove all record of the
				// association."
				case AssociationState::SHUTDOWN_SENT:
				case AssociationState::SHUTDOWN_ACK_SENT:
				{
					auto packet                 = this->tcb->CreatePacket();
					auto* shutdownCompleteChunk = packet->BuildChunkInPlace<ShutdownCompleteChunk>();

					// NOTE: Don't set bit T in the SHUTDOWN_COMPLETE chunk since TCB
					// knows the Verification Tag expected by the remote.

					shutdownCompleteChunk->Consolidate();

					this->packetSender.SendPacket(packet.get());

					InternalClose(Types::ErrorKind::NO_ERROR, "");

					break;
				}

				// https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
				//
				// "If the receiver is in COOKIE-ECHOED or COOKIE-WAIT state, the
				// procedures in Section 8.4 SHOULD be followed; in other words, it is
				// treated as an OOTB packet."
				//
				// https://datatracker.ietf.org/doc/html/rfc9260#section-8.4
				//
				// "If the packet contains a SHUTDOWN ACK chunk, the receiver SHOULD
				// respond to the sender of the OOTB packet with a SHUTDOWN COMPLETE
				// chunk. When sending the SHUTDOWN COMPLETE chunk, the receiver of the
				// OOTB packet MUST fill in the Verification Tag field of the outbound
				// packet with the Verification Tag received in the SHUTDOWN ACK chunk
				// and set the T bit in the Chunk Flags to indicate that the
				// Verification Tag is reflected."
				default:
				{
					auto packet = this->CreatePacketWithVerificationTag(receivedPacket->GetVerificationTag());
					auto* shutdownCompleteChunk = packet->BuildChunkInPlace<ShutdownCompleteChunk>();

					shutdownCompleteChunk->SetT(true);
					shutdownCompleteChunk->Consolidate();

					this->packetSender.SendPacket(packet.get());
				}
			}
		}

		void Socket::ProcessReceivedShutdownCompleteChunk(
		  const Packet* /*receivedPacket*/, const ShutdownCompleteChunk* /*receivedShutdownCompleteChunk*/)
		{
			MS_TRACE();

			if (this->associationState != AssociationState::SHUTDOWN_ACK_SENT)
			{
				return;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "Upon reception of the SHUTDOWN COMPLETE chunk, the endpoint verifies
			// that it is in the SHUTDOWN-ACK-SENT state; if it is not, the chunk
			// SHOULD be discarded. If the endpoint is in the SHUTDOWN-ACK-SENT state,
			// the endpoint SHOULD stop the T2-shutdown timer and remove all knowledge
			// of the association (and thus the association enters the CLOSED state)."
			InternalClose(Types::ErrorKind::NO_ERROR, "");
		}

		void Socket::ProcessReceivedOperationErrorChunk(
		  const Packet* /*receivedPacket*/, const OperationErrorChunk* receivedOperationErrorChunk)
		{
			MS_TRACE();

			std::string errorCausesStr;

			errorCausesStr.reserve(50);

			for (auto it = receivedOperationErrorChunk->ErrorCausesBegin();
			     it != receivedOperationErrorChunk->ErrorCausesEnd();
			     ++it)
			{
				const auto* errorCause = *it;

				if (!errorCausesStr.empty())
				{
					errorCausesStr.append(", ");
				}

				errorCausesStr.append(errorCause->ToString());
			}

			if (!this->tcb)
			{
				MS_DEBUG_TAG(
				  sctp,
				  "received OPERATION_ERROR Chunk on a Socket with no TCB, ignoring: %s",
				  errorCausesStr.c_str());

				return;
			}

			MS_WARN_TAG(sctp, "received OPERATION_ERROR Chunk: %s", errorCausesStr.c_str());

			this->listener.OnSocketError(Types::ErrorKind::PEER_REPORTED, errorCausesStr);
		}

		void Socket::ProcessReceivedAbortAssociationChunk(
		  const Packet* /*receivedPacket*/, const AbortAssociationChunk* receivedAbortAssociationChunk)
		{
			MS_TRACE();

			std::string errorCausesStr;

			errorCausesStr.reserve(50);

			for (auto it = receivedAbortAssociationChunk->ErrorCausesBegin();
			     it != receivedAbortAssociationChunk->ErrorCausesEnd();
			     ++it)
			{
				const auto* errorCause = *it;

				if (!errorCausesStr.empty())
				{
					errorCausesStr.append(", ");
				}

				errorCausesStr.append(errorCause->ToString());
			}

			if (!this->tcb)
			{
				MS_DEBUG_TAG(
				  sctp, "received ABORT Chunk on a Socket with no TCB, ignoring: %s", errorCausesStr.c_str());

				return;
			}

			MS_WARN_TAG(sctp, "received ABORT Chunk, closing connection: %s", errorCausesStr.c_str());

			InternalClose(Types::ErrorKind::PEER_REPORTED, errorCausesStr);
		}

		void Socket::ProcessReceivedHeartbeatRequestChunk(
		  const Packet* /*receivedPacket*/, const HeartbeatRequestChunk* receivedHeartbeatRequestChunk)
		{
			MS_TRACE();

			if (!ValidateHasTcb())
			{
				return;
			}

			// TODO: Implement it.
			// this->tcb->GetHearbeatHandler().HandleHeartbeatRequest(*std::move(receivedHeartbeatRequestChunk));
		}

		void Socket::ProcessReceivedHeartbeatAckChunk(
		  const Packet* /*receivedPacket*/, const HeartbeatAckChunk* receivedHeartbeatAckChunk)
		{
			MS_TRACE();

			if (!ValidateHasTcb())
			{
				return;
			}

			// TODO: Implement it.
			// this->tcb->GetHearbeatHandler().HandleHeartbeatAck(*std::move(receivedHeartbeatAckChunk));
		}

		void Socket::ProcessReceivedReConfigChunk(
		  const Packet* /*receivedPacket*/, const ReConfigChunk* receivedReConfigChunk)
		{
			MS_TRACE();

			if (!ValidateHasTcb())
			{
				return;
			}

			// TODO: Implement it.
			// this->tcb->GetStreamResetHandler().HandleReConfig(*std::move(receivedReConfigChunk));

			// Handling this response may result in outgoing stream resets finishing
			// (either successfully or with failure). If there still are pending
			// streams that were waiting for this request to finish, continue
			// resetting them.
			MaySendResetStreamsRequest();

			// If a response was processed, pending to-be-reset streams may now have
			// become unpaused. Try to send more DATA/I_DATA chunks.
			// TODO: Implement it.
			// this->tcb->SendBufferedPackets(callbacks_.Now());

			// If it leaves "deferred reset processing", there may be chunks to
			// deliver that were queued while waiting for the stream to reset.
			MayDeliverMessages();
		}

		void Socket::ProcessReceivedForwardTsnChunk(
		  const Packet* receivedPacket, const ForwardTsnChunk* receivedForwardTsnChunk)
		{
			MS_TRACE();

			ProcessReceivedAnyForwardTsnChunk(receivedPacket, receivedForwardTsnChunk);
		}

		void Socket::ProcessReceivedIForwardTsnChunk(
		  const Packet* receivedPacket, const IForwardTsnChunk* receivedIForwardTsnChunk)
		{
			MS_TRACE();

			ProcessReceivedAnyForwardTsnChunk(receivedPacket, receivedIForwardTsnChunk);
		}

		void Socket::ProcessReceivedAnyForwardTsnChunk(
		  const Packet* /*receivedPacket*/, const AnyForwardTsnChunk* receivedAnyForwardTsnChunk)
		{
			MS_TRACE();

			if (!ValidateHasTcb())
			{
				return;
			}

			if (!this->tcb->GetNegotiatedCapabilities().partialReliability)
			{
				auto packet                 = this->tcb->CreatePacket();
				auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: Don't set bit T in the ABORT chunk since TCB knows the
				// Verification Tag expected by the remote.

				auto* protocolViolationErrorCause =
				  abortAssociationChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "FORWARD_TSN or I_FORWARD_TSN-TSN chunk received but partial reliability is not negotiated");

				protocolViolationErrorCause->Consolidate();
				abortAssociationChunk->Consolidate();

				this->packetSender.SendPacket(packet.get());

				this->listener.OnSocketError(
				  Types::ErrorKind::PROTOCOL_VIOLATION,
				  "received FORWARD_TSN or I_FORWARD_TSN-TSN chunk but partial reliability is not negotiated");

				return;
			}

			// TODO: Implement it.
			// if
			// (this->tcb->GetDataTracker().HandleForwardTsn(receivedAnyForwardTsnChunk->GetNewCumulativeTsn()))
			// {
			// 	this->tcb->GetReassemblyQueue().HandleForwardTsn(
			// 		receivedAnyForwardTsnChunk->GetNewCumulativeTsn(),
			// 		receivedAnyForwardTsnChunk->GetSkippedStreams());
			// }

			// A forward TSN (for ordered streams) may allow messages to be delivered.
			MayDeliverMessages();
		}

		void Socket::ProcessReceivedDataChunk(const Packet* receivedPacket, const DataChunk* receivedDataChunk)
		{
			MS_TRACE();

			ProcessReceivedAnyDataChunk(receivedPacket, receivedDataChunk);
		}

		void Socket::ProcessReceivedIDataChunk(
		  const Packet* receivedPacket, const IDataChunk* receivedIDataChunk)
		{
			MS_TRACE();

			ProcessReceivedAnyDataChunk(receivedPacket, receivedIDataChunk);
		}

		void Socket::ProcessReceivedAnyDataChunk(
		  const Packet* receivedPacket, const AnyDataChunk* receivedAnyDataChunk)
		{
			MS_TRACE();

			if (!ValidateHasTcb())
			{
				return;
			}

			const uint32_t tsn      = receivedAnyDataChunk->GetTsn();
			const bool immediateAck = receivedAnyDataChunk->GetI();

			if (receivedAnyDataChunk->GetUserDataLength() == 0)
			{
				auto packet               = this->tcb->CreatePacket();
				auto* operationErrorChunk = packet->BuildChunkInPlace<OperationErrorChunk>();
				auto* noUserDataErrorCause =
				  operationErrorChunk->BuildErrorCauseInPlace<NoUserDataErrorCause>();

				noUserDataErrorCause->SetTsn(tsn);
				noUserDataErrorCause->Consolidate();
				operationErrorChunk->Consolidate();

				this->packetSender.SendPacket(packet.get());

				this->listener.OnSocketError(
				  Types::ErrorKind::PROTOCOL_VIOLATION, "received DATA or I_DATA chunk with no user data");

				return;
			}

			// TODO: Implement it.
			// MS_DEBUG_DEV("data received [data length:%" PRIu16 ", queue size:%zu, watermark:%zu,
			// full:%s, above:%s]", 	receivedAnyDataChunk->GetUserDataLength(),
			// 	this->tcb->GetReassemblyQueue()->GetQueuedBytes(),
			// 	this->tcb->GetReassemblyQueue()->GetWaterMarkBytes(),
			// 	this->tcb->GetReassemblyQueue()->IsFull(),
			// 	this->tcb->GetReassemblyQueue()->IsAboveWatermark(),
			// );

			// TODO: Implement it.
			// if (this->tcb->GetReassemblyQueue()->IsFull())
			// {
			// 	// If the reassembly queue is full but there are assembled messages
			// 	// waiting to be pulled, we can't do anything with this data except drop
			// 	// it, and hope the upper layer drains the accumulated messages soon.
			// 	if (this->tcb->GetReassemblyQueue()->HasMessages())
			// 	{
			// 		MS_WARN_TAG(sctp, "received data rejected because reassembly queue is full");

			// 		return;
			// 	}
			// 	// If the reassembly queue is full and there's no messages waiting,
			// 	// there is nothing that can be done. The specification only allows
			// 	// dropping gap-ack-blocks, and that's not likely to help as the socket
			// 	// has been trying to fill gaps since the watermark was reached.
			// 	else
			// 	{
			// 		auto packet      = this->tcb->CreatePacket();
			// 		auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

			// 		// NOTE: Don't set bit T in the ABORT chunk since TCB knows the
			// 		// Verification Tag expected by the remote.

			// 		auto* outOfResourceErrorCause =
			// 		  abortAssociationChunk->BuildErrorCauseInPlace<OutOfResourceErrorCause>();

			// 		outOfResourceErrorCause->Consolidate();
			// 		abortAssociationChunk->Consolidate();

			// 		this->packetSender.SendPacket(packet.get());

			// 		InternalClose(Types::ErrorKind::RESOURCE_EXHAUSTION, "reassembly queue is exhausted");

			// 		return;
			// 	}
			// }

			// If the reassembly queue is above its high watermark, only accept data
			// chunks that increase its cumulative ack tsn in an attempt to fill gaps
			// to deliver messages.
			// TODO: Implement it.
			// if (this->tcb->GetReassemblyQueue()->IsAboveWatermark())
			// {
			// 	MS_WARN_TAG(sctp, "reassembly queue is above watermark");

			// 	if (this->tcb->GetDataTracker()->WillIncreaseCumAckTsn(tsn))
			// 	{
			// 		MS_WARN_TAG(sctp, "reassembly queue is above watermark");

			// 		this->tcb->GetDataTracker()->ForceImmediateSack();

			// 		return;
			// 	}
			// }

			// TODO: Implement it.
			// if (this->tcb->GetDataTracker()->IsTsnValid(tsn))
			// {
			// 	MS_WARN_TAG(sctp, "data rejected because of failing TSN validity");

			// 	return;
			// }

			// TODO: Implement it.
			// if (this->tcb->GetDataTracker()->Observe(tsn, immediateAck))
			// {
			// 	// TODO: Here we should have a std::vector<uint8_t> holding the data so
			// 	// we can move it.
			// 	this->tcb->GetReassemblyQueue()->Add(tsn, std::move(data));

			// 	MayDeliverMessages();
			// }
		}

		void Socket::ProcessReceivedSackChunk(
		  const Packet* /*receivedPacket*/, const SackChunk* receivedSackChunk)
		{
			MS_TRACE();

			const uint64_t now = DepLibUV::GetTimeMs();

			// TODO
		}

		bool Socket::ProcessReceivedUnknownChunk(
		  const Packet* /*receivedPacket*/, const UnknownChunk* receivedUnknownChunk)
		{
			MS_TRACE();

			const auto action         = receivedUnknownChunk->GetActionForUnknownChunkType();
			const bool skipProcessing = action == Chunk::ActionForUnknownChunkType::SKIP ||
			                            action == Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT;
			const bool reportError = action == Chunk::ActionForUnknownChunkType::STOP_AND_REPORT ||
			                         action == Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT;

			if (skipProcessing)
			{
				MS_DEBUG_TAG(
				  sctp,
				  "Chunk with unknown type %" PRIu8
				  " received, skipping further processing of Chunks in the Packet",
				  static_cast<uint8_t>(receivedUnknownChunk->GetType()));
			}
			else
			{
				MS_DEBUG_TAG(
				  sctp,
				  "ignoring received Chunk with unknown type %" PRIu8,
				  static_cast<uint8_t>(receivedUnknownChunk->GetType()));
			}

			if (reportError)
			{
				this->listener.OnSocketError(
				  Types::ErrorKind::PARSE_FAILED, "unknown chunk with type indicating it should be reported");

				// If there is TCB (we need correct remote verification tag) send an
				// OPERATION_ERROR Chunk with a Unrecognized Chunk Type Error Cause.
				if (this->tcb)
				{
					auto packet               = this->tcb->CreatePacket();
					auto* operationErrorChunk = packet->BuildChunkInPlace<OperationErrorChunk>();
					auto* unrecognizedChunkTypeErrorCause =
					  operationErrorChunk->BuildErrorCauseInPlace<UnrecognizedChunkTypeErrorCause>();

					unrecognizedChunkTypeErrorCause->SetUnrecognizedChunk(
					  receivedUnknownChunk->GetBuffer(), receivedUnknownChunk->GetLength());

					unrecognizedChunkTypeErrorCause->Consolidate();
					operationErrorChunk->Consolidate();

					this->packetSender.SendPacket(packet.get());
				}
			}

			return !skipProcessing;
		}

		void Socket::OnT1InitTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp,
			  "T1-init timer has expired %zu/%zu]",
			  this->t1InitTimer->GetExpirationCount(),
			  this->t1InitTimer->GetMaxRestarts());

			AssertAssociationState(AssociationState::COOKIE_WAIT);

			if (this->t1InitTimer->IsRunning())
			{
				SendInitChunk();
			}
			else
			{
				InternalClose(Types::ErrorKind::TOO_MANY_RETRIES, "no INIT_ACK chunk received");
			}

			AssertAssociationStateIsConsistent();
		}

		void Socket::OnT1CookieTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp,
			  "T1-cookie timer has expired %zu/%zu]",
			  this->t1CookieTimer->GetExpirationCount(),
			  this->t1CookieTimer->GetMaxRestarts());

			AssertAssociationState(AssociationState::COOKIE_ECHOED);

			if (this->t1CookieTimer->IsRunning())
			{
				// TODO: Implement it.
				// this->tcb->SendBufferedPackets(now);
			}
			else
			{
				InternalClose(Types::ErrorKind::TOO_MANY_RETRIES, "no COOKIE_ACK chunk received");
			}

			AssertAssociationStateIsConsistent();
		}

		void Socket::OnT2ShutdownTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp,
			  "T2-shutdown timer has expired %zu/%zu]",
			  this->t2ShutdownTimer->GetExpirationCount(),
			  this->t2ShutdownTimer->GetMaxRestarts());

			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "If the timer expires, the endpoint MUST resend the SHUTDOWN chunk
			// with the updated last sequential TSN received from its peer."
			if (this->t2ShutdownTimer->IsRunning())
			{
				SendShutdownChunk();
			}
			// https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			//
			// "An endpoint SHOULD limit the number of retransmissions of the
			// SHUTDOWN chunk to the protocol parameter 'Association.Max.Retrans'. If
			// this threshold is exceeded, the endpoint SHOULD destroy the TCB and
			// SHOULD report the peer endpoint unreachable to the upper layer (and
			// thus the association enters the CLOSED state)."
			else
			{
				AssertHasTcb();

				auto packet                 = this->tcb->CreatePacket();
				auto* abortAssociationChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: Don't set bit T in the ABORT chunk since TCB knows the
				// Verification Tag expected by the remote.

				auto* userInitiatedAbortErrorCause =
				  abortAssociationChunk->BuildErrorCauseInPlace<UserInitiatedAbortErrorCause>();

				userInitiatedAbortErrorCause->SetUpperLayerAbortReason(
				  "too many retransmissions of SHUTDOWN chunk");

				userInitiatedAbortErrorCause->Consolidate();
				abortAssociationChunk->Consolidate();

				this->packetSender.SendPacket(packet.get());

				InternalClose(Types::ErrorKind::TOO_MANY_RETRIES, "no SHUTDOWN_ACK chunk received");
			}

			AssertAssociationStateIsConsistent();
		}

		template<typename... AssociationStates>
		void Socket::AssertAssociationState(AssociationStates... expectedAssociationStates) const
		{
			MS_TRACE();

			static_assert(
			  (std::is_same_v<AssociationStates, AssociationState> && ...),
			  "all arguments must be of type AssociationState");

			// NOTE: Using fold expression operator.
			if ((... || (this->associationState == expectedAssociationStates)))
			{
				return;
			}

			const auto currentAssociationStateStringView =
			  Socket::AssociationStateToString(this->associationState);
			std::ostringstream expectedAssociationStatesOss;
			bool firstExpectedAssociationState = true;

			// NOTE: Using fold expression operator.
			((expectedAssociationStatesOss << (firstExpectedAssociationState ? "" : ", ")
			                               << Socket::AssociationStateToString(expectedAssociationStates),
			  firstExpectedAssociationState = false),
			 ...);

			auto expectedAssociationStatesString = expectedAssociationStatesOss.str();

			MS_ABORT(
			  "current association state %.*s does not match any of the given expected states (%s)",
			  static_cast<int>(currentAssociationStateStringView.size()),
			  currentAssociationStateStringView.data(),
			  expectedAssociationStatesString.c_str());
		}

		template<typename... AssociationStates>
		void Socket::AssertNotAssociatonState(AssociationStates... unexpectedAssociationStates) const
		{
			MS_TRACE();

			static_assert(
			  (std::is_same_v<AssociationStates, AssociationState> && ...),
			  "all arguments must be of type AssociationState");

			// NOTE: Using fold expression operator.
			if ((... || (this->associationState == unexpectedAssociationStates)))
			{
				const auto currentAssociationStateStringView =
				  Socket::AssociationStateToString(this->associationState);
				std::ostringstream unexpectedAssociationStatesOss;
				bool firstUnexpectedAssociationState = true;

				// NOTE: Using fold expression operator.
				((unexpectedAssociationStatesOss
				    << (firstUnexpectedAssociationState ? "" : ", ")
				    << Socket::AssociationStateToString(unexpectedAssociationStates),
				  firstUnexpectedAssociationState = false),
				 ...);

				const auto unexpectedAssociationStatesString = unexpectedAssociationStatesOss.str();

				MS_ABORT(
				  "current association state %.*s matches one of the given unexpected states (%s)",
				  static_cast<int>(currentAssociationStateStringView.size()),
				  currentAssociationStateStringView.data(),
				  unexpectedAssociationStatesString.c_str());
			}
		}

		bool Socket::ValidateHasTcb()
		{
			MS_TRACE();

			if (this->tcb)
			{
				return true;
			}

			this->listener.OnSocketError(
			  Types::ErrorKind::NOT_CONNECTED,
			  "received unexpected commands on socket that is not connected");

			return false;
		}

		void Socket::AssertHasTcb() const
		{
			MS_TRACE();

			if (!this->tcb)
			{
				MS_ABORT("TCB doesn't exist");
			}
		}

		void Socket::AssertAssociationStateIsConsistent() const
		{
			MS_TRACE();

			switch (this->associationState)
			{
				case AssociationState::CLOSED:
				{
					MS_ASSERT(!this->tcb, "association state is CLOSED but there is TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is CLOSED but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is CLOSED but T1 Cookie timer is running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is CLOSED but T2 Shutdown timer is running");

					break;
				}

				case AssociationState::COOKIE_WAIT:
				{
					MS_ASSERT(!this->tcb, "association state is COOKIE_WAIT but there is TCB");
					MS_ASSERT(
					  this->t1InitTimer->IsRunning(),
					  "association state is COOKIE_WAIT but T1 Init timer is not running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is COOKIE_WAIT but T1 Cookie timer is running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is COOKIE_WAIT but T2 Shutdown timer is running");

					break;
				}

				case AssociationState::COOKIE_ECHOED:
				{
					MS_ASSERT(this->tcb, "association state is COOKIE_ECHOED but there is no TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is COOKIE_ECHOED but T1 Init timer is not running");
					MS_ASSERT(
					  this->t1CookieTimer->IsRunning(),
					  "association state is COOKIE_ECHOED but T1 Cookie timer is not running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is COOKIE_ECHOED but T2 Shutdown timer is running");
					// TODO: Implement this.
					// MS_ASSERT(this->tcb->HasCookieEchoChunk(), "association state is COOKIE_ECHOED but TCB
					// does't have ECHO chunk");

					break;
				}

				case AssociationState::ESTABLISHED:
				{
					MS_ASSERT(this->tcb, "association state is ESTABLISHED but there is not TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is ESTABLISHED but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is ESTABLISHED but T1 Cookie timer is running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is ESTABLISHED but T2 Shutdown timer is running");

					break;
				}

				case AssociationState::SHUTDOWN_PENDING:
				{
					MS_ASSERT(this->tcb, "association state is SHUTDOWN_PENDING but there is not TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is SHUTDOWN_PENDING but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is SHUTDOWN_PENDING but T1 Cookie timer is running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is SHUTDOWN_PENDING but T2 Shutdown timer is running");

					break;
				}

				case AssociationState::SHUTDOWN_SENT:
				{
					MS_ASSERT(this->tcb, "association state is SHUTDOWN_SENT but there is not TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is SHUTDOWN_SENT but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is SHUTDOWN_SENT but T1 Cookie timer is running");
					MS_ASSERT(
					  this->t2ShutdownTimer->IsRunning(),
					  "association state is SHUTDOWN_SENT but T2 Shutdown timer is not running");

					break;
				}

				case AssociationState::SHUTDOWN_RECEIVED:
				{
					MS_ASSERT(this->tcb, "association state is SHUTDOWN_RECEIVED but there is not TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is SHUTDOWN_RECEIVED but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is SHUTDOWN_RECEIVED but T1 Cookie timer is running");
					MS_ASSERT(
					  !this->t2ShutdownTimer->IsRunning(),
					  "association state is SHUTDOWN_RECEIVED but T2 Shutdown timer is running");

					break;
				}

				case AssociationState::SHUTDOWN_ACK_SENT:
				{
					MS_ASSERT(this->tcb, "association state is SHUTDOWN_ACK_SENT but there is not TCB");
					MS_ASSERT(
					  !this->t1InitTimer->IsRunning(),
					  "association state is SHUTDOWN_ACK_SENT but T1 Init timer is running");
					MS_ASSERT(
					  !this->t1CookieTimer->IsRunning(),
					  "association state is SHUTDOWN_ACK_SENT but T1 Cookie timer is running");
					MS_ASSERT(
					  this->t2ShutdownTimer->IsRunning(),
					  "association state is SHUTDOWN_ACK_SENT but T2 Shutdown timer is not running");

					break;
				}
			}
		}

		void Socket::OnPacketSenderPacketSent(
		  PacketSender* /*packetSender*/, const Packet* /*packet*/, bool sent)
		{
			MS_TRACE();

			if (sent)
			{
				this->privateMetrics.txPacketsCount++;
			}
		}

		void Socket::OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->t1InitTimer.get())
			{
				OnT1InitTimer(baseTimeoutMs, stop);
			}
			else if (backoffTimer == this->t1CookieTimer.get())
			{
				OnT1CookieTimer(baseTimeoutMs, stop);
			}
			else if (backoffTimer == this->t2ShutdownTimer.get())
			{
				OnT2ShutdownTimer(baseTimeoutMs, stop);
			}
		}
	} // namespace SCTP
} // namespace RTC
