#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Socket.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/StateCookie.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
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

		thread_local static uint8_t FactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

		/* Class methods. */

		constexpr std::string_view Socket::StateToString(Socket::State state)
		{
			// NOTE: We cannot use MS_TRACE() here because clang in Linux will
			// comlain about "read of non-constexpr variable 'configuration' is not
			// allowed in a constant expression".

			switch (state)
			{
				case Socket::State::CLOSED:
					return "CLOSED";
				case Socket::State::COOKIE_WAIT:
					return "COOKIE_WAIT";
				case Socket::State::COOKIE_ECHOED:
					return "COOKIE_ECHOED";
				case Socket::State::ESTABLISHED:
					return "ESTABLISHED";
				case Socket::State::SHUTDOWN_PENDING:
					return "SHUTDOWN_PENDING";
				case Socket::State::SHUTDOWN_SENT:
					return "SHUTDOWN_SENT";
				case Socket::State::SHUTDOWN_RECEIVED:
					return "SHUTDOWN_RECEIVED";
				case Socket::State::SHUTDOWN_ACK_SENT:
					return "SHUTDOWN_ACK_SENT";
			}
		}

		/* Instance methods. */

		Socket::Socket(SocketOptions options, SocketListener* listener)
		  : options(options), listener(listener),
		    t1InitTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ options.t1InitTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ options.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ options.maxInitRetransmissions)),
		    t1CookieTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ options.t1CookieTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ options.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ options.maxInitRetransmissions)),
		    t2ShutdownTimer(
		      std::make_unique<BackoffTimerHandle>(
		        /*listener*/ this,
		        /*baseTimeoutMs*/ options.t2ShutdownTimeoutMs,
		        /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		        /*maxBackoffTimeout*/ options.timerMaxBackoffTimeoutMs,
		        /*maxRestarts*/ options.maxRetransmissions))
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

			auto stateStringView = Socket::StateToString(this->state);

			MS_DUMP_CLEAN(indentation, "<SCTP::Socket>");
			MS_DUMP_CLEAN(
			  indentation, "  state: %.*s", static_cast<int>(stateStringView.size()), stateStringView.data());
			this->metrics.Dump(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::Socket>");
		}

		void Socket::Connect()
		{
			MS_TRACE();

			if (this->state != State::CLOSED)
			{
				const auto stateStringView = Socket::StateToString(this->state);

				MS_DEBUG_TAG(
				  sctp,
				  "cannot initiate the association since state is not CLOSED but %.*s",
				  static_cast<int>(stateStringView.size()),
				  stateStringView.data());

				return;
			}

			this->preTcb.localVerificationTag =
			  Utils::Crypto::GetRandomUInt<uint32_t>(1, std::numeric_limits<uint32_t>::max());
			this->preTcb.localInitialTsn =
			  Utils::Crypto::GetRandomUInt<uint32_t>(0, std::numeric_limits<uint32_t>::max());

			SendInitChunk();

			this->t1InitTimer->Start();

			SetState(State::COOKIE_WAIT, "Connect() called");
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

		void Socket::SetState(State state, const std::string& reason)
		{
			MS_TRACE();

			const auto stateStringView = Socket::StateToString(state);

			if (state == this->state)
			{
				MS_WARN_TAG(
				  sctp,
				  "Socket state is already %.*s (reason:'%s')",
				  static_cast<int>(stateStringView.size()),
				  stateStringView.data(),
				  reason.c_str());

				return;
			}

			const auto previousStateStringView = Socket::StateToString(this->state);

			MS_WARN_TAG(
			  sctp,
			  "Socket state changed from %.*s to %.*s (reason:'%s')",
			  static_cast<int>(previousStateStringView.size()),
			  previousStateStringView.data(),
			  static_cast<int>(stateStringView.size()),
			  stateStringView.data(),
			  reason.c_str());

			this->state = state;
		}

		void Socket::AddCapabilitiesParametersToInitOrInitAckChunk(Chunk* chunk) const
		{
			MS_TRACE();

			MS_ASSERT(
			  chunk->GetType() == Chunk::ChunkType::INIT || chunk->GetType() == Chunk::ChunkType::INIT_ACK,
			  "chunk must be an INIT or INIT_ACK");

			auto* supportedExtensionsParameter =
			  chunk->BuildParameterInPlace<SupportedExtensionsParameter>();

			supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);

			if (this->options.enablePartialReliability)
			{
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::FORWARD_TSN);

				auto* forwardTsnSupportedParameter =
				  chunk->BuildParameterInPlace<ForwardTsnSupportedParameter>();

				forwardTsnSupportedParameter->Consolidate();
			}

			if (this->options.enableMessageInterleaving)
			{
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_DATA);
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::I_FORWARD_TSN);
			}

			if (
			  this->options.zeroChecksumAlternateErrorDetectionMethod !=
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE)
			{
				auto* zeroChecksumAcceptableParameter =
				  chunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

				zeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
				  this->options.zeroChecksumAlternateErrorDetectionMethod);
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
			  localVerificationTag,
			  remoteVerificationTag,
			  localInitialTsn,
			  remoteInitialTsn,
			  remoteAdvertisedReceiverWindowCredit,
			  tieTag,
			  negotiatedCapabilities);

			this->metrics.messageInterleaving = negotiatedCapabilities.messageInterleaving;
			this->metrics.zeroChecksum        = negotiatedCapabilities.zeroChecksum;
		}

		std::unique_ptr<Packet> Socket::CreatePacket() const
		{
			MS_TRACE();

			uint32_t verificationTag = this->tcb ? this->tcb->GetRemoteVerificationTag() : 0;

			return CreatePacketWithVerificationTag(verificationTag);
		}

		std::unique_ptr<Packet> Socket::CreatePacketWithVerificationTag(uint32_t verificationTag) const
		{
			MS_TRACE();

			auto packet = std::unique_ptr<Packet>(Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer)));

			packet->SetSourcePort(this->options.sourcePort);
			packet->SetDestinationPort(this->options.destinationPort);
			packet->SetVerificationTag(verificationTag);

			return packet;
		}

		void Socket::SendPacket(Packet* packet, std::optional<bool> writeChecksum)
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
			this->listener.OnSocketSendSctpPacket(this, packet);
		}

		void Socket::SendInitChunk()
		{
			MS_TRACE();

			auto packet = CreatePacket();

			// Insert an INIT Chunk in the Packet.
			auto* initChunk = packet->BuildChunkInPlace<InitChunk>();

			initChunk->SetInitiateTag(this->preTcb.localVerificationTag);
			initChunk->SetAdvertisedReceiverWindowCredit(this->options.maxReceiverWindowBufferSize);
			initChunk->SetNumberOfOutboundStreams(this->options.maxOutboundStreams);
			initChunk->SetNumberOfInboundStreams(this->options.maxInboundStreams);
			initChunk->SetInitialTsn(this->preTcb.localInitialTsn);

			// Insert capabilities related Parameters in the INIT Chunk.
			AddCapabilitiesParametersToInitOrInitAckChunk(initChunk);

			initChunk->Consolidate();

			// https://datatracker.ietf.org/doc/html/rfc9653#section-5.2
			// When a sender sends a packet containing an INIT chunk, it MUST include
			// a correct CRC32c checksum in the packet containing the INIT chunk.
			SendPacket(packet.get(), /*writeChecksum*/ true);
		}

		void Socket::SendShutdownAckChunk()
		{
			MS_TRACE();

			auto packet            = CreatePacket();
			auto* shutdownAckChunk = packet->BuildChunkInPlace<ShutdownAckChunk>();

			shutdownAckChunk->Consolidate();

			SendPacket(packet.get());

			// TODO
			// this->t2ShutdownTimer->SetBaseTimeout(this->tcb->GetCurrentRto());
			this->t2ShutdownTimer->Restart();
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

				// TODO
				// InternalClose(ErrorKind::kProtocolViolation, "Received invalid INIT Chunk");

				return;
			}

			// "If an endpoint is in the SHUTDOWN-ACK-SENT state and receives an INIT
			// chunk (e.g., if the SHUTDOWN COMPLETE chunk was lost) with source and
			// destination transport addresses (either in the IP addresses or in the
			// INIT chunk) that belong to this association, it SHOULD discard the
			// INIT chunk and retransmit the SHUTDOWN ACK chunk."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#section-9.2
			if (this->state == State::SHUTDOWN_ACK_SENT)
			{
				MS_DEBUG_TAG(
				  sctp, "INIT Chunk received in SHUTDOWN_ACK_SENT state, retransmitting SHUTDOWN_ACK Chunk");

				SendShutdownAckChunk();

				return;
			}

			uint64_t tieTag{ 0 };
			uint32_t localVerificationTag;
			uint32_t localInitialTsn;

			switch (this->state)
			{
				case State::CLOSED:
				{
					MS_DEBUG_TAG(sctp, "INIT Chunk received in CLOSED state (normal scenario)");

					localVerificationTag =
					  Utils::Crypto::GetRandomUInt<uint32_t>(1, std::numeric_limits<uint32_t>::max());
					localInitialTsn =
					  Utils::Crypto::GetRandomUInt<uint32_t>(0, std::numeric_limits<uint32_t>::max());

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
				case State::COOKIE_WAIT:
				case State::COOKIE_ECHOED:
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
					  Utils::Crypto::GetRandomUInt<uint32_t>(1, std::numeric_limits<uint32_t>::max());

					// TODO: Implement this.
					// Make the initial TSN make a large jump, so that there is no overlap
					// with the old and new association.
					// my_initial_tsn = TSN(*tcb_->retransmission_queue().next_tsn() + 1000000);

					// TODO: Remove this when the above TODO is done.
					localInitialTsn =
					  Utils::Crypto::GetRandomUInt<uint32_t>(0, std::numeric_limits<uint32_t>::max());

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
			initAckChunk->SetAdvertisedReceiverWindowCredit(this->options.maxReceiverWindowBufferSize);
			initAckChunk->SetNumberOfOutboundStreams(this->options.maxOutboundStreams);
			initAckChunk->SetNumberOfInboundStreams(this->options.maxInboundStreams);
			initAckChunk->SetInitialTsn(localInitialTsn);

			// Insert a StateCookie Parameter in the INIT_ACK Chunk.
			auto* stateCookieParameter = initAckChunk->BuildParameterInPlace<StateCookieParameter>();

			const auto negotiatedCapabilities =
			  NegotiatedCapabilities::Factory(this->options, receivedInitChunk);

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
			if (this->state != State::COOKIE_WAIT)
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

				// TODO
				// InternalClose(ErrorKind::kProtocolViolation, "received INIT_ACK Chunk doesn't contain a Cookie");

				return;
			}

			this->metrics.peerImplementation = StateCookie::DetermineSctpImplementation(
			  stateCookieParameter->GetCookie(), stateCookieParameter->GetCookieLength());

			this->t1InitTimer->Stop();

			const auto negotiatedCapabilities =
			  NegotiatedCapabilities::Factory(this->options, receivedInitAckChunk);

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

			SetState(State::COOKIE_ECHOED, "INIT_ACK received");

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
			auto reportError    = action == Chunk::ActionForUnknownChunkType::STOP_AND_REPORT ||
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

		void Socket::OnT1InitTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp, "T1-init timer expired [timeout count:%zu]", this->t1InitTimer->GetTimeoutCount());

			AssertState(State::COOKIE_WAIT);

			if (this->t1InitTimer->IsActive())
			{
				SendInitChunk();
			}
			else
			{
				// TODO
				// InternalClose(ErrorKind::kTooManyRetries, "No INIT_ACK received");
			}
		}

		void Socket::OnT1CookieTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp, "T1-cookie timer expired [timeout count:%zu]", this->t1CookieTimer->GetTimeoutCount());

			AssertState(State::COOKIE_ECHOED);

			if (this->t1CookieTimer->IsActive())
			{
				// TODO
				// tcb_->SendBufferedPackets(callbacks_.Now());
			}
			else
			{
				// TODO
				// InternalClose(ErrorKind::kTooManyRetries, "No COOKIE_ACK received");
			}
		}

		void Socket::OnT2ShutdownTimer(uint64_t& baseTimeoutMs, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(
			  sctp,
			  "T2-shutdown timer expired [timeout count:%zu]",
			  this->t2ShutdownTimer->GetTimeoutCount());

			// TODO
		}

		template<typename... States>
		void Socket::AssertState(States... expectedStates) const
		{
			MS_TRACE();

			static_assert((std::is_same_v<States, State> && ...), "all arguments must be of type State");

			// NOTE: Using fold expression operator.
			if ((... || (this->state == expectedStates)))
			{
				return;
			}

			auto currentStateStringView = Socket::StateToString(this->state);
			std::ostringstream expectedStatesOss;
			bool firstExpectedState = true;

			// NOTE: Using fold expression operator.
			((expectedStatesOss << (firstExpectedState ? "" : ", ") << Socket::StateToString(expectedStates),
			  firstExpectedState = false),
			 ...);

			auto expectedStatesString = expectedStatesOss.str();

			MS_ABORT(
			  "current Socket state %.*s does not match any of the given expected states (%s)",
			  static_cast<int>(currentStateStringView.size()),
			  currentStateStringView.data(),
			  expectedStatesString.c_str());
		}

		template<typename... States>
		void Socket::AssertNotState(States... unexpectedStates) const
		{
			MS_TRACE();

			static_assert((std::is_same_v<States, State> && ...), "all arguments must be of type State");

			// NOTE: Using fold expression operator.
			if ((... || (this->state == unexpectedStates)))
			{
				auto currentStateStringView = Socket::StateToString(this->state);
				std::ostringstream unexpectedStatesOss;
				bool firstUnexpectedState = true;

				// NOTE: Using fold expression operator.
				((unexpectedStatesOss << (firstUnexpectedState ? "" : ", ")
				                      << Socket::StateToString(unexpectedStates),
				  firstUnexpectedState = false),
				 ...);

				auto unexpectedStatesString = unexpectedStatesOss.str();

				MS_ABORT(
				  "current Socket state %.*s matches one of the given unexpected states (%s)",
				  static_cast<int>(currentStateStringView.size()),
				  currentStateStringView.data(),
				  unexpectedStatesString.c_str());
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
