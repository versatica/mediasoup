#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
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

			SendInit();

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

		void Socket::SendInit()
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
			packet->SetCRC32cChecksum();

			// Send the Packet.
			this->listener->OnSocketSendSctpPacket(this, packet);

			delete packet;
		}

		void Socket::OnT1InitTimer(uint64_t& baseTimeout, bool& stop)
		{
			MS_TRACE();

			MS_DEBUG_TAG(sctp, "T1-timer expired [timeout count:%zu]", this->t1InitTimer->GetTimeoutCount());

			AssertState(State::COOKIE_WAIT);

			if (this->t1InitTimer->IsActive())
			{
				SendInit();
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
			  sctp, "T1-coookie expired [timeout count:%zu]", this->t1CookieTimer->GetTimeoutCount());

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
			  sctp, "T2-shutdown expired [timeout count:%zu]", this->t2ShutdownTimer->GetTimeoutCount());

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
