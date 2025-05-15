#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
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

		constexpr std::string_view Socket::State2String(Socket::State state)
		{
			MS_TRACE();

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

		Socket::Socket(SocketOptions options, Listener* listener)
		  : options(options), listener(listener),
		    t1InitTimer(std::make_unique<BackoffTimerHandle>(
		      /*listener*/ this,
		      /*baseTimeout*/ options.t1InitTimeout,
		      /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		      /*maxBackoffTimeout*/ options.timerMaxBackoffTimeout,
		      /*maxRestarts*/ options.maxInitRetransmits)),
		    t1CookieTimer(std::make_unique<BackoffTimerHandle>(
		      /*listener*/ this,
		      /*baseTimeout*/ options.t1CookieTimeout,
		      /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		      /*maxBackoffTimeout*/ options.timerMaxBackoffTimeout,
		      /*maxRestarts*/ options.maxInitRetransmits)),
		    t2ShutdownTimer(std::make_unique<BackoffTimerHandle>(
		      /*listener*/ this,
		      /*baseTimeout*/ options.t2ShutdownTimeout,
		      /*backoffAlgorithm*/ BackoffTimerHandle::BackoffAlgorithm::EXPONENTIAL,
		      /*maxBackoffTimeout*/ options.timerMaxBackoffTimeout,
		      /*maxRestarts*/ options.maxRetransmits))
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

			// TODO

			auto stateStringView = Socket::State2String(this->state);

			MS_DUMP_CLEAN(indentation, "<SCTP::Socket>");
			MS_DUMP_CLEAN(
			  indentation, "  state: %.*s", static_cast<int>(stateStringView.size()), stateStringView.data());
			this->metrics.Dump(indentation);
			MS_DUMP_CLEAN(indentation, "</SCTP::Socket>");
		}

		void Socket::Associate()
		{
			MS_TRACE();

			// TODO: Is this ok? We Associate() is public API and parent should be
			// able to invoke it despite the connection is already established after
			// being initiated from the rmeote peer.
			// TODO: Are we gonna notify the parent with SCTP state changes? I assume
			// yes. But does it mean that parent should know in which states it can
			// invoke certain public methods?
			AssertNotState(State::CLOSED);

			this->preTcb.localVerificationTag =
			  Utils::Crypto::GetRandomUInt(1, std::numeric_limits<uint32_t>::max());
			this->preTcb.localInitialTsn =
			  Utils::Crypto::GetRandomUInt(0, std::numeric_limits<uint32_t>::max());

			SendInitChunk();

			this->t1InitTimer->Start();

			SetState(State::COOKIE_WAIT, "Associate() called");
		}

		// TODO: Should the caller call free packet after calling this method? or us?
		void Socket::ReceivePacket(const Packet* packet)
		{
			MS_TRACE();

			this->metrics.rxPacketsCount++;

			/* Verify Packet. */

			if (!ValidateReceivedPacket(packet))
			{
				MS_WARN_TAG(sctp, "Packet verification failed, discarded");

				return;
			}

			// TODO
			// MaybeSendShutdownOnPacketReceived(*packet);

			for (auto it = packet->ChunksBegin(); it != packet->ChunksEnd(); ++it)
			{
				const auto* chunk = *it;

				if (!ProcessReceivedChunk(packet, chunk))
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

			auto stateStringView = Socket::State2String(state);

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

			auto previousStateStringView = Socket::State2String(this->state);

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

		void Socket::AddCapabilitiesParametersToChunk(Chunk* chunk) const
		{
			MS_TRACE();

			auto* supportedExtensionsParameter =
			  chunk->BuildParameterInPlace<SupportedExtensionsParameter>();

			supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::RE_CONFIG);

			if (this->options.partialReliability)
			{
				supportedExtensionsParameter->AddChunkType(Chunk::ChunkType::FORWARD_TSN);

				auto* forwardTsnSupportedParameter =
				  chunk->BuildParameterInPlace<ForwardTsnSupportedParameter>();

				forwardTsnSupportedParameter->Consolidate();
			}

			if (this->options.messageInterleaving)
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
		  uint32_t localAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		{
			MS_TRACE();

			this->tcb = std::make_unique<TransmissionControlBlock>(
			  localVerificationTag,
			  remoteVerificationTag,
			  localInitialTsn,
			  remoteInitialTsn,
			  localAdvertisedReceiverWindowCredit,
			  tieTag,
			  negotiatedCapabilities);

			this->metrics.messageInterleaving = negotiatedCapabilities.messageInterleaving;
			this->metrics.zeroChecksum        = negotiatedCapabilities.zeroChecksum;
		}

		Packet* Socket::CreatePacket() const
		{
			MS_TRACE();

			uint32_t remoteVerificationTag = this->tcb ? this->tcb->GetRemoteVerificationTag() : 0;

			return CreatePacketWithRemoteVerificationTag(remoteVerificationTag);
		}

		Packet* Socket::CreatePacketWithRemoteVerificationTag(uint32_t remoteVerificationTag) const
		{
			MS_TRACE();

			auto* packet = Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer));

			packet->SetSourcePort(this->options.sourcePort);
			packet->SetDestinationPort(this->options.destinationPort);
			packet->SetVerificationTag(remoteVerificationTag);

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
			this->listener->OnSocketSendSctpPacket(this, packet);
		}

		void Socket::SendInitChunk()
		{
			MS_TRACE();

			AssertState(State::CLOSED);

			auto* packet    = CreatePacket();
			auto* initChunk = packet->BuildChunkInPlace<InitChunk>();

			initChunk->SetInitiateTag(this->preTcb.localVerificationTag);
			initChunk->SetAdvertisedReceiverWindowCredit(this->options.localAdvertisedReceiverWindowCredit);
			initChunk->SetNumberOfOutboundStreams(this->options.maxOutboundStreams);
			initChunk->SetNumberOfInboundStreams(this->options.maxInboundStreams);
			initChunk->SetInitialTsn(this->preTcb.localInitialTsn);

			AddCapabilitiesParametersToChunk(initChunk);

			initChunk->Consolidate();

			// https://datatracker.ietf.org/doc/html/rfc9653#section-5.2
			// When a sender sends a packet containing an INIT chunk, it MUST include
			// a correct CRC32c checksum in the packet containing the INIT chunk.
			SendPacket(packet, /*writeChecksum*/ true);

			delete packet;
		}

		void Socket::SendShutdownAckChunk()
		{
			MS_TRACE();

			AssertState(State::CLOSED);

			auto* packet           = CreatePacket();
			auto* shutdownAckChunk = packet->BuildChunkInPlace<ShutdownAckChunk>();

			shutdownAckChunk->Consolidate();

			SendPacket(packet);

			delete packet;

			// TODO
			// this->t2ShutdownTimer->SetBaseTimeout(this->tcb->GetCurrentRto());
			this->t2ShutdownTimer->Restart();
		}

		bool Socket::ValidateReceivedPacket(const Packet* packet)
		{
			MS_TRACE();

			uint32_t localVerificationTag = this->tcb ? this->tcb->GetLocalVerificationTag() : 0;

			// "When an endpoint receives an SCTP packet with the Verification Tag
			// set to 0, it SHOULD verify that the packet contains only an INIT
			// chunk. Otherwise, the receiver MUST silently discard the packet."
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#name-exceptions-in-verification-
			if (packet->GetVerificationTag() == 0)
			{
				if (packet->GetChunksCount() == 1 && packet->GetChunkAt(0)->GetType() == Chunk::ChunkType::INIT)
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

			if (packet->GetChunksCount() >= 1 && packet->GetChunkAt(0)->GetType() == Chunk::ChunkType::INIT_ACK)
			{
				if (packet->GetVerificationTag() == this->preTcb.localVerificationTag)
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "invalid Verification Tag %" PRIu32 " (should be %" PRIu32 ")",
					  packet->GetVerificationTag(),
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
			if (packet->GetChunksCount() == 1 && packet->GetChunkAt(0)->GetType() == Chunk::ChunkType::ABORT)
			{
				auto* abortChunk = static_cast<const AbortAssociationChunk*>(packet->GetChunkAt(0));

				// We cannot verify the Verification Tag so assume it's okey.
				if (abortChunk->GetT() && !this->tcb)
				{
					return true;
				}
				else if (
				  (!abortChunk->GetT() && packet->GetVerificationTag() == localVerificationTag) ||
				  (abortChunk->GetT() &&
				   packet->GetVerificationTag() == this->tcb->GetRemoteVerificationTag()))
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "ABORT Chunk Verification Tag %" PRIu32 " is wrong, packet discarded",
					  packet->GetVerificationTag());

					// TODO: Emit error?
					return false;
				}
			}

			// This is handled in Chunk handler.
			//
			// @see https://datatracker.ietf.org/doc/html/rfc9260#name-handle-a-cookie-echo-chunk-
			if (packet->GetChunksCount() >= 1 && packet->GetChunkAt(0)->GetType() == Chunk::ChunkType::COOKIE_ECHO)
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
			if (packet->GetChunksCount() == 1 && packet->GetChunkAt(0)->GetType() == Chunk::ChunkType::SHUTDOWN_COMPLETE)
			{
				auto* shutdownCompleteChunk =
				  static_cast<const ShutdownCompleteChunk*>(packet->GetChunkAt(0));

				// We cannot verify the Verification Tag so assume it's okey.
				if (shutdownCompleteChunk->GetT() && !this->tcb)
				{
					return true;
				}
				else if (
				  (!shutdownCompleteChunk->GetT() && packet->GetVerificationTag() == localVerificationTag) ||
				  (shutdownCompleteChunk->GetT() &&
				   packet->GetVerificationTag() == this->tcb->GetRemoteVerificationTag()))
				{
					return true;
				}
				else
				{
					MS_WARN_TAG(
					  sctp,
					  "SHUTDOWN_COMPLETE Chunk Verification Tag %" PRIu32 " is wrong, packet discarded",
					  packet->GetVerificationTag());

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
			if (packet->GetVerificationTag() == localVerificationTag)
			{
				return true;
			}
			else
			{
				MS_WARN_TAG(
				  sctp,
				  "invalid Verification Tag %" PRIu32 " (should be %" PRIu32 ")",
				  packet->GetVerificationTag(),
				  localVerificationTag);

				// TODO: Emit error?
				return false;
			}

			// TODO
		}

		bool Socket::ProcessReceivedChunk(const Packet* packet, const Chunk* chunk)
		{
			MS_TRACE();

			switch (chunk->GetType())
			{
				case Chunk::ChunkType::DATA:
				{
					ProcessReceivedDataChunk(packet, static_cast<const DataChunk*>(chunk));

					break;
				}

				case Chunk::ChunkType::INIT:
				{
					ProcessReceivedInitChunk(packet, static_cast<const InitChunk*>(chunk));

					break;
				}

				case Chunk::ChunkType::INIT_ACK:
				{
					ProcessReceivedInitAckChunk(packet, static_cast<const InitAckChunk*>(chunk));

					break;
				}

				default:
				{
					return ProcessReceivedUnknownChunk(packet, static_cast<const UnknownChunk*>(chunk));
				}
			}

			return true;
		}

		void Socket::ProcessReceivedDataChunk(const Packet* packet, const DataChunk* chunk)
		{
			MS_TRACE();
		}

		void Socket::ProcessReceivedInitChunk(const Packet* packet, const InitChunk* chunk)
		{
			MS_TRACE();

			// Verify some fields that cannot be 0.
			if (
			  chunk->GetInitiateTag() == 0 || chunk->GetNumberOfOutboundStreams() == 0 or
			  chunk->GetNumberOfInboundStreams() == 0)
			{
				MS_WARN_TAG(
				  sctp,
				  "invalid value 0 in Initiate Tag or Number of Outbound Streams or Number of Inbound Streams in received INIT Chunk, aborting association");

				auto* packet     = CreatePacketWithRemoteVerificationTag(0);
				auto* abortChunk = packet->BuildChunkInPlace<AbortAssociationChunk>();

				// NOTE: We are not setting the Verification Tag expected by the peer
				// so must set be T to 1.
				abortChunk->SetT(true);

				auto* protocolViolationErrorCause =
				  abortChunk->BuildErrorCauseInPlace<ProtocolViolationErrorCause>();

				protocolViolationErrorCause->SetAdditionalInformation(
				  "invalid value 0 in Initiate Tag or Number of Outbound Streams or Number of Inbound Streams in received INIT Chunk");

				protocolViolationErrorCause->Consolidate();
				abortChunk->Consolidate();

				SendPacket(packet);

				delete packet;

				// TODO
				// InternalClose(ErrorKind::kProtocolViolation, "Received invalid INIT");

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
					  Utils::Crypto::GetRandomUInt(1, std::numeric_limits<uint32_t>::max());
					localInitialTsn = Utils::Crypto::GetRandomUInt(0, std::numeric_limits<uint32_t>::max());

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
					  Utils::Crypto::GetRandomUInt(1, std::numeric_limits<uint32_t>::max());
					tieTag = this->tcb->GetTieTag();
				}
			}

			MS_DEBUG_TAG(
			  sctp,
			  "initiating association [localVerificationTag:%" PRIu32 ", localInitialTsn:%" PRIu32
			  ", remoteVerificationTag:%" PRIu32 ", remoteInitialTsn:%" PRIu32 "]",
			  localVerificationTag,
			  localInitialTsn,
			  chunk->GetInitiateTag(),
			  chunk->GetInitialTsn());

			// auto negotiatedCapabilities = NegotiatedCapabilities::Factory(
			//   this->options, chunk->GetNumberOfOutboundStreams(), chunk->GetNumberOfInboundStreams(), chunk);

			// TODO: More
		}

		void Socket::ProcessReceivedInitAckChunk(const Packet* packet, const InitAckChunk* chunk)
		{
			MS_TRACE();
		}

		bool Socket::ProcessReceivedUnknownChunk(const Packet* /*packet*/, const UnknownChunk* chunk)
		{
			MS_TRACE();

			auto action         = chunk->GetActionForUnknownChunkType();
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
				  static_cast<uint8_t>(chunk->GetType()));
			}
			else
			{
				MS_DEBUG_TAG(
				  sctp,
				  "ignoring received Chunk with unknown type %" PRIu8,
				  static_cast<uint8_t>(chunk->GetType()));
			}

			if (reportError)
			{
				// TODO: Notify error.

				if (this->tcb)
				{
					// TODO: Send an ErrorChunk with Unrecognized Chunk Type.
				}
			}

			return !skipProcessing;
		}

		void Socket::OnT1InitTimer(uint64_t& baseTimeout, bool& stop)
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

		void Socket::OnT1CookieTimer(uint64_t& baseTimeout, bool& stop)
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

		void Socket::OnT2ShutdownTimer(uint64_t& baseTimeout, bool& stop)
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

			auto currentStateStringView = Socket::State2String(this->state);
			std::ostringstream expectedStatesOss;
			bool firstExpectedState = true;

			// NOTE: Using fold expression operator.
			((expectedStatesOss << (firstExpectedState ? "" : ", ") << Socket::State2String(expectedStates),
			  firstExpectedState = false),
			 ...);

			auto expectedStatesString = expectedStatesOss.str();

			MS_THROW_ERROR(
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
				auto currentStateStringView = Socket::State2String(this->state);
				std::ostringstream unexpectedStatesOss;
				bool firstUnexpectedState = true;

				// NOTE: Using fold expression operator.
				((unexpectedStatesOss << (firstUnexpectedState ? "" : ", ")
				                      << Socket::State2String(unexpectedStates),
				  firstUnexpectedState = false),
				 ...);

				auto unexpectedStatesString = unexpectedStatesOss.str();

				MS_THROW_ERROR(
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
				MS_THROW_ERROR("TCB doesn't exist");
			}
		}

		void Socket::OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeout, bool& stop)
		{
			MS_TRACE();

			if (backoffTimer == this->t1InitTimer.get())
			{
				OnT1InitTimer(baseTimeout, stop);
			}
			else if (backoffTimer == this->t1CookieTimer.get())
			{
				OnT1CookieTimer(baseTimeout, stop);
			}
			else if (backoffTimer == this->t2ShutdownTimer.get())
			{
				OnT2ShutdownTimer(baseTimeout, stop);
			}
		}
	} // namespace SCTP
} // namespace RTC
