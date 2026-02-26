#include "RTC/SCTP/Types.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include <cstdint>
#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/Socket.hpp"
#include "RTC/SCTP/StateCookie.hpp"
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

		/* Instance methods. */

		Socket::Socket(const SctpOptions& sctpOptions, SocketListener& listener)
		  : sctpOptions(sctpOptions), listener(listener),
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

			auto associationStateStringView = Socket::AssociationStateToString(this->associationState);

			MS_DUMP_CLEAN(indentation, "<SCTP::Socket>");

			MS_DUMP_CLEAN(
			  indentation,
			  "  association state: %.*s",
			  static_cast<int>(associationStateStringView.size()),
			  associationStateStringView.data());

			this->metrics.Dump(indentation + 1);

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

					SendPacket(packet.get());
				}

				InternalClose(Types::ErrorKind::NO_ERROR, "");
			}
			else
			{
				MS_DEBUG_TAG(sctp, "called on a closed Socket");
			}

			AssertAssociationStateIsConsistent();
		}

		std::optional<SocketMetrics> Socket::GetMetrics() const
		{
			if (!this->tcb)
			{
				return std::nullopt;
			}

			return ComputeMetrics();
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

		void Socket::SetBufferedAmountLowThreshold(uint16_t stream_id, size_t bytes)
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
				  Types::ErrorKind::WRONG_SEQUENCE, "can't reset streams as the socket is not connected");

				return Types::ResetStreamsStatus::NOT_CONNECTED;
			}

			// TODO: Implement it.
			// if (!this->tcb->GetCapabilities().reconfig)
			// {
			//   this->listener.OnSocketError(Types::ErrorKind::UNSUPPORTED_OPERATION,
			//                      "can't reset streams as the remote doesn't support it");

			//   return Types::ResetStreamsStatus::NOT_SUPPORTED;
			// }

			// TODO: Implement it.
			// this->tcb->GetStreamResetHandler().ResetStreams(outboundStreamIds);

			// TODO: Implement it.
			// MaySendResetStreamsRequest();

			AssertAssociationStateIsConsistent();

			return Types::ResetStreamsStatus::PERFORMED;
		}

		// TODO: Why not Message&?
		Types::SendMessageStatus Socket::SendMessage(
		  Message message, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			// TODO: Implement it.
			Types::SendMessageStatus status = InternalSendMessage(message, sendMessageOptions);

			if (status != Types::SendMessageStatus::SUCCESS)
			{
				return status;
			}

			const uint64_t now = DepLibUV::GetTimeMs();

			this->metrics.txMessagesCount++;

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

		// TODO: Why not Message&?
		std::vector<Types::SendMessageStatus> Socket::SendManyMessages(
		  std::span<Message> messages, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			SocketDeferredListener::ScopedDeferred deferrer(this->listener);

			const uint64_t now = DepLibUV::GetTimeMs();
			std::vector<Types::SendMessageStatus> statuses;

			statuses.reserve(messages.size());

			for (auto& message : messages)
			{
				// TODO: Implement it.
				Types::SendMessageStatus status = InternalSendMessage(message, sendMessageOptions);

				statuses.push_back(status);

				if (status != Types::SendMessageStatus::SUCCESS)
				{
					continue;
				}

				this->metrics.txMessagesCount++;

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

		// TODO: Should the caller call free packet after calling this method? or us?
		void Socket::ReceivePacket(const Packet* receivedPacket)
		{
			MS_TRACE();

			this->metrics.rxPacketsCount++;

			/* Verify Packet. */

			if (!ValidateReceivedPacket(receivedPacket))
			{
				MS_WARN_TAG(sctp, "Packet verification failed, discarded");

				return;
			}

			// TODO
			// MaybeSendShutdownOnPacketReceived(receivedPacket);

			for (auto it = receivedPacket->ChunksBegin(); it != receivedPacket->ChunksEnd(); ++it)
			{
				const auto* receivedChunk = *it;

				if (!ProcessReceivedChunk(receivedPacket, receivedChunk))
				{
					break;
				}
			}

			// TODO
			// if (tcb_ != nullptr) {
			//   tcb_->data_tracker().ObservePacketEnd();
			//   tcb_->MaybeSendSack();
			// }
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
			  this->sctpOptions,
			  localVerificationTag,
			  remoteVerificationTag,
			  localInitialTsn,
			  remoteInitialTsn,
			  remoteAdvertisedReceiverWindowCredit,
			  tieTag,
			  negotiatedCapabilities);

			this->metrics.negotiatedMaxOutboundStreams = negotiatedCapabilities.maxOutboundStreams;
			this->metrics.negotiatedMaxInboundStreams  = negotiatedCapabilities.maxInboundStreams;
			this->metrics.usesPartialReliability       = negotiatedCapabilities.partialReliability;
			this->metrics.usesMessageInterleaving      = negotiatedCapabilities.messageInterleaving;
			this->metrics.usesReconfig                 = negotiatedCapabilities.reconfig;
			this->metrics.usesZeroChecksum             = negotiatedCapabilities.zeroChecksum;
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

		bool Socket::SendPacket(Packet* packet, std::optional<bool> writeChecksum)
		{
			MS_TRACE();

			// Decide whether to write the CRC32c Checksum field in the Packet or
			// not. Note that in same special cases the decision is made by the
			// caller of SendPacket() which explicitly sets the value of the
			// `writeChecksum` argument.

			// If `writeChecksum` is explicitly set to true then write the checksum.
			if (writeChecksum.has_value() && *writeChecksum)
			{
				packet->WriteCRC32cChecksum();
			}
			// If `writeChecksum` is explicitly set to false then do not write the
			// checksum.
			else if (writeChecksum.has_value() && !*writeChecksum)
			{
				// Nothing to do.
			}
			// If `writeChecksum` is not set, decide based on TCB.
			else if (!this->tcb || !this->tcb->GetNegotiatedCapabilities().zeroChecksum)
			{
				packet->WriteCRC32cChecksum();
			}

			// Send the Packet.
			return this->listener.OnSocketSendSctpPacket(packet);
		}

		Types::SendMessageStatus Socket::InternalSendMessage(
		  const Message& message, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			// TODO
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
			// When a sender sends a packet containing an INIT chunk, it MUST include
			// a correct CRC32c checksum in the packet containing the INIT chunk.
			SendPacket(packet.get(), /*writeChecksum*/ true);
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

			auto packet            = CreatePacket();
			auto* shutdownAckChunk = packet->BuildChunkInPlace<ShutdownAckChunk>();

			shutdownAckChunk->Consolidate();

			SendPacket(packet.get());

			// TODO
			// this->t2ShutdownTimer->SetBaseTimeout(this->tcb->GetCurrentRto());
			this->t2ShutdownTimer->Restart();
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
				this->t2ShutdownTimer->Restart();

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

		bool Socket::ValidateReceivedPacket(const Packet* receivedPacket)
		{
			MS_TRACE();

			uint32_t localVerificationTag = this->tcb ? this->tcb->GetLocalVerificationTag() : 0;

			// "When an endpoint receives an SCTP packet with the Verification Tag
			// set to 0, it SHOULD verify that the packet contains only an INIT
			// chunk. Otherwise, the receiver MUST silently discard the packet."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#name-exceptions-in-verification-
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

					// TODO: Emit error?
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
					  "invalid Verification Tag %" PRIu32 " (should be %" PRIu32 ")",
					  receivedPacket->GetVerificationTag(),
					  this->preTcb.localVerificationTag);

					// TODO: Emit error?
					return false;
				}
			}

			// "The receiver of an ABORT chunk MUST accept the packet if the
			// Verification Tag field of the packet matches its own tag and the T bit
			// is not set OR if it is set to its Peer's Tag and the T bit is set in
			// the Chunk Flags. Otherwise, the receiver MUST silently discard the
			// packet and take no further action."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
			if (receivedPacket->GetChunksCount() == 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::ABORT)
			{
				auto* abortChunk = static_cast<const AbortAssociationChunk*>(receivedPacket->GetChunkAt(0));

				// We cannot verify the Verification Tag so assume it's okey.
				if (abortChunk->GetT() && !this->tcb)
				{
					return true;
				}
				else if (
				  (!abortChunk->GetT() && receivedPacket->GetVerificationTag() == localVerificationTag) ||
				  (abortChunk->GetT() &&
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

					// TODO: Emit error?
					return false;
				}
			}

			// This is handled in ProcessCookieEchoChunk().
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#name-handle-a-cookie-echo-chunk-
			if (receivedPacket->GetChunksCount() >= 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::COOKIE_ECHO)
			{
				return true;
			}

			// "The receiver of a SHUTDOWN COMPLETE shall accept the packet if the
			// Verification Tag field of the packet matches its own tag and the T bit is
			// not set OR if it is set to its peer's tag and the T bit is set in the
			// Chunk Flags.  Otherwise, the receiver MUST silently discard the packet
			// and take no further action."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#section-8.5.1
			if (receivedPacket->GetChunksCount() == 1 && receivedPacket->GetChunkAt(0)->GetType() == Chunk::ChunkType::SHUTDOWN_COMPLETE)
			{
				auto* shutdownCompleteChunk =
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

					// TODO: Emit error?
					return false;
				}
			}

			// "When receiving an SCTP packet, the endpoint MUST ensure that the
			// value in the Verification Tag field of the received SCTP packet
			// matches its own tag. If the received Verification Tag value does not
			// match the receiver's own tag value, the receiver MUST silently discard
			// the packet and MUST NOT process it any further, except for those cases
			// listed in Section 8.5.1 below."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#section-8.5
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

				// TODO: Emit error?
				return false;
			}
		}

		bool Socket::ProcessReceivedChunk(const Packet* receivedPacket, const Chunk* receivedChunk)
		{
			MS_TRACE();

			switch (receivedChunk->GetType())
			{
				case Chunk::ChunkType::DATA:
				{
					ProcessReceivedDataChunk(receivedPacket, static_cast<const DataChunk*>(receivedChunk));

					break;
				}

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

				case Chunk::ChunkType::SACK:
				{
					ProcessReceivedSackChunk(receivedPacket, static_cast<const SackChunk*>(receivedChunk));

					break;
				}

				case Chunk::ChunkType::HEARTBEAT_REQUEST:
				{
					ProcessReceivedHeartbeatRequestChunk(
					  receivedPacket, static_cast<const HeartbeatRequestChunk*>(receivedChunk));

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

		void Socket::ProcessReceivedDataChunk(const Packet* receivedPacket, const DataChunk* receivedDataChunk)
		{
			MS_TRACE();

			// TODO
		}

		void Socket::ProcessReceivedInitChunk(
		  const Packet* /*receivedPacket*/, const InitChunk* receivedInitChunk)
		{
			MS_TRACE();

			// Verify some fields that cannot be 0.
			if (
			  receivedInitChunk->GetInitiateTag() == 0 ||
			  receivedInitChunk->GetNumberOfOutboundStreams() == 0 or
			  receivedInitChunk->GetNumberOfInboundStreams() == 0)
			{
				MS_WARN_TAG(
				  sctp,
				  "invalid value 0 in Initiate Tag or Number of Outbound Streams or Number of Inbound Streams in received INIT Chunk, aborting association");

				auto packet      = CreatePacketWithVerificationTag(0);
				auto* abortChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: We are not setting the Verification Tag expected by the peer
				// so must set be T to 1.
				abortChunk->SetT(true);

				auto* protocolViolationErrorCause =
				  abortChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "invalid value 0 in Initiate Tag or Number of Outbound Streams or Number of Inbound Streams in received INIT chunk");

				protocolViolationErrorCause->Consolidate();
				abortChunk->Consolidate();

				SendPacket(packet.get());

				InternalClose(Types::ErrorKind::PROTOCOL_VIOLATION, "received invalid INIT chunk");

				return;
			}

			// "If an endpoint is in the SHUTDOWN-ACK-SENT state and receives an INIT
			// chunk (e.g., if the SHUTDOWN COMPLETE chunk was lost) with source and
			// destination transport addresses (either in the IP addresses or in the
			// INIT chunk) that belong to this association, it SHOULD discard the
			// INIT chunk and retransmit the SHUTDOWN ACK chunk."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
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

				/**
				 * "This usually indicates an initialization collision, i.e., each
				 * endpoint is attempting, at about the same time, to establish an
				 * association with the other endpoint. Upon receipt of an INIT chunk
				 * in the COOKIE-WAIT state, an endpoint MUST respond with an INIT ACK
				 * chunk using the same parameters it sent in its original INIT chunk
				 * (including its Initiate Tag, unchanged)."
				 *
				 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.1
				 */
				case AssociationState::COOKIE_WAIT:
				case AssociationState::COOKIE_ECHOED:
				{
					MS_DEBUG_TAG(sctp, "INIT Chunk received after sending INIT Chunk (collision, no problem)");

					localVerificationTag = this->preTcb.localVerificationTag;
					localInitialTsn      = this->preTcb.localInitialTsn;

					break;
				}

				/**
				 * "The outbound SCTP packet containing this INIT ACK chunk MUST carry
				 * a Verification Tag value equal to the Initiate Tag found in the
				 * unexpected INIT chunk. And the INIT ACK chunk MUST contain a new
				 * Initiate Tag (randomly generated; see Section 5.3.1). Other
				 * parameters for the endpoint SHOULD be copied from the existing
				 * parameters of the association (e.g., number of outbound streams)
				 * into the INIT ACK chunk and cookie."
				 *
				 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-5.2.2
				 */
				default:
				{
					AssertHasTcb();

					MS_DEBUG_TAG(sctp, "INIT Chunk received (probably peer restarted)");

					localVerificationTag =
					  Utils::Crypto::GetRandomUInt<uint32_t>(MinVerificationTag, MaxVerificationTag);

					// TODO: Implement this.
					// Make the initial TSN make a large jump, so that there is no overlap
					// with the old and new association.
					// my_initial_tsn = TSN(*tcb_->retransmission_queue().next_tsn() + 1000000);

					// TODO: Remove this when the above TODO is done.
					localInitialTsn = Utils::Crypto::GetRandomUInt<uint32_t>(MinInitialTsn, MaxInitialTsn);

					tieTag = this->tcb->GetTieTag();
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

			// Insert a StateCookie Parameter in the INIT_ACK Chunk.
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

			SendPacket(packet.get(), /*writeChecksum*/ !negotiatedCapabilities.zeroChecksum);
		}

		void Socket::ProcessReceivedInitAckChunk(
		  const Packet* /*receivedPacket*/, const InitAckChunk* receivedInitAckChunk)
		{
			MS_TRACE();

			// "If an INIT ACK chunk is received by an endpoint in any state other
			// than the COOKIE-WAIT or CLOSED state, the endpoint SHOULD discard the
			// INIT ACK chunk."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#name-unexpected-init-ack-chunk
			if (this->associationState != AssociationState::COOKIE_WAIT)
			{
				MS_DEBUG_TAG(sctp, "ignoring received INIT_ACK Chunk when not in COOKIE_WAIT state");

				return;
			}

			auto* stateCookieParameter =
			  receivedInitAckChunk->GetFirstParameterOfType<StateCookieParameter>();

			if (!stateCookieParameter || !stateCookieParameter->GetCookie())
			{
				MS_WARN_TAG(
				  sctp, "ignoring received INIT_ACK Chunk without StateCookieParameter or without Cookie");

				auto packet      = CreatePacketWithVerificationTag(this->preTcb.localVerificationTag);
				auto* abortChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: We are not setting the Verification Tag expected by the peer
				// so must set be T to 1.
				abortChunk->SetT(true);

				auto* protocolViolationErrorCause =
				  abortChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "INIT_ACK without State Cookie Parameter or without Cookie");

				protocolViolationErrorCause->Consolidate();
				abortChunk->Consolidate();

				SendPacket(packet.get());

				InternalClose(
				  Types::ErrorKind::PROTOCOL_VIOLATION, "received INIT_ACK chunk doesn't contain a Cookie");

				return;
			}

			this->metrics.peerImplementation = StateCookie::DetermineSctpImplementation(
			  stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength());

			this->t1InitTimer->Stop();

			const auto negotiatedCapabilities =
			  NegotiatedCapabilities::Factory(this->sctpOptions, receivedInitAckChunk);

			// TODO
			// If the connection is re-established (peer restarted, but re-used old
			// connection), make sure that all message identifiers are reset and any
			// partly sent message is re-sent in full. The same is true when the socket
			// is closed and later re-opened, which never happens in WebRTC, but is a
			// valid operation on the SCTP level. Note that in case of handover, the
			// send queue is already re-configured, and shouldn't be reset.
			// send_queue_.Reset();

			CreateTransmissionControlBlock(
			  this->preTcb.localVerificationTag,
			  receivedInitAckChunk->GetInitiateTag(),
			  this->preTcb.localInitialTsn,
			  receivedInitAckChunk->GetInitialTsn(),
			  receivedInitAckChunk->GetAdvertisedReceiverWindowCredit(),
			  /*tieTag*/ Utils::Crypto::GetRandomUInt<uint64_t>(0, std::numeric_limits<uint64_t>::max()),
			  negotiatedCapabilities);

			SetAssociationState(AssociationState::COOKIE_ECHOED, "INIT_ACK received");

			// TODO
			// The connection isn't fully established just yet.
			// tcb_->SetCookieEchoChunk(CookieEchoChunk(cookie->data()));
			// tcb_->SendBufferedPackets(callbacks_.Now());

			this->t1CookieTimer->Start();
		}

		void Socket::ProcessReceivedSackChunk(
		  const Packet* /*receivedPacket*/, const SackChunk* receivedSackChunk)
		{
			MS_TRACE();

			// TODO
		}

		void Socket::ProcessReceivedHeartbeatRequestChunk(
		  const Packet* /*receivedPacket*/, const HeartbeatRequestChunk* receivedHeartbeatRequestChunk)
		{
			MS_TRACE();

			// TODO
		}

		bool Socket::ProcessReceivedUnknownChunk(
		  const Packet* /*receivedPacket*/, const UnknownChunk* receivedUnknownChunk)
		{
			MS_TRACE();

			auto action         = receivedUnknownChunk->GetActionForUnknownChunkType();
			auto skipProcessing = action == Chunk::ActionForUnknownChunkType::SKIP ||
			                      action == Chunk::ActionForUnknownChunkType::SKIP_AND_REPORT;
			auto reportError = action == Chunk::ActionForUnknownChunkType::STOP_AND_REPORT ||
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
				// TODO: Notify error.

				// If there is TCB (we need correct remote verification tag) send an
				// OPERATION_ERROR Chunk with a Unrecognized Chunk Type Error Cause.
				if (this->tcb)
				{
					auto packet               = CreatePacket();
					auto* operationErrorChunk = packet->BuildChunkInPlace<OperationErrorChunk>();
					auto* unrecognizedChunkTypeErrorCause =
					  operationErrorChunk->BuildErrorCauseInPlace<UnrecognizedChunkTypeErrorCause>();

					unrecognizedChunkTypeErrorCause->SetUnrecognizedChunk(
					  receivedUnknownChunk->GetBuffer(), receivedUnknownChunk->GetLength());

					unrecognizedChunkTypeErrorCause->Consolidate();
					operationErrorChunk->Consolidate();

					SendPacket(packet.get());
				}
			}

			return !skipProcessing;
		}

		SocketMetrics Socket::ComputeMetrics() const
		{
			MS_TRACE();

			SocketMetrics metrics = this->metrics;

			if (!this->tcb)
			{
				return metrics;
			}

			// const size_t packetPayloadLength =
			//   this->sctpOptions.mtu - Packet::CommonHeaderLength - DataChunk::DataChunkHeaderLength;

			// TODO: Implement it.
			// metrics.cwndBytes = this->tcb->getCwnd();
			// metrics.srtt_ms = this->tcb->getCurrentSrttMs();
			// metrics.unackDataCount =
			//   this->tcb->getRetransmissionQueue().GetUnackedItems() +
			//   (this->sendQueue.getTotalBufferedAmount() + packetPayloadLength - 1) / packetPayloadLength;
			// metrics.peerRwndBytes = this->tcb->getRetransmissionQueue().getRwnd();
			// metrics.negotiatedMaxOutboundStreams =
			//   this->tcb->GetCapabilities().negotiatedMaxOutboundStreams;
			// metrics.negotiatedMaxInboundStreams = this->tcb->GetCapabilities().negotiatedMaxInboundStreams;
			// metrics.rtxPacketsCount = this->tcb->getRetransmissionQueue().getRtxPacketsCount();
			// metrics.rtxBytesCount   = this->tcb->getRetransmissionQueue().getRtxBytesCount();

			return metrics;
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

				SendPacket(packet.get());

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

			auto currentAssociationStateStringView =
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
				auto currentAssociationStateStringView =
				  Socket::AssociationStateToString(this->associationState);
				std::ostringstream unexpectedAssociationStatesOss;
				bool firstUnexpectedAssociationState = true;

				// NOTE: Using fold expression operator.
				((unexpectedAssociationStatesOss
				    << (firstUnexpectedAssociationState ? "" : ", ")
				    << Socket::AssociationStateToString(unexpectedAssociationStates),
				  firstUnexpectedAssociationState = false),
				 ...);

				auto unexpectedAssociationStatesString = unexpectedAssociationStatesOss.str();

				MS_ABORT(
				  "current association state %.*s matches one of the given unexpected states (%s)",
				  static_cast<int>(currentAssociationStateStringView.size()),
				  currentAssociationStateStringView.data(),
				  unexpectedAssociationStatesString.c_str());
			}
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
