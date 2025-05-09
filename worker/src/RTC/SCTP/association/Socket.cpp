#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
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

		Socket::Socket(SocketOptions options, Listener* listener) : options(options), listener(listener)
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
		}

		void Socket::Associate()
		{
			MS_TRACE();

			AssertNotState(State::CLOSED);

			this->preTcb.myVerificationTag =
			  Utils::Crypto::GetRandomUInt(1, std::numeric_limits<uint32_t>::max());
			this->preTcb.myInitialTsn =
			  Utils::Crypto::GetRandomUInt(0, std::numeric_limits<uint32_t>::max());

			SendInit();

			// TODO
			// t1_init_->Start();

			SetState(State::COOKIE_WAIT, "Associate() called");
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

			uint32_t peerVerificationTag = this->tcb ? this->tcb->GetPeerVerificationTag() : 0;

			return CreatePacketWithPeerVerificationTag(peerVerificationTag);
		}

		Packet* Socket::CreatePacketWithPeerVerificationTag(uint32_t peerVerificationTag) const
		{
			MS_TRACE();

			auto* packet = Packet::Factory(FactoryBuffer, sizeof(FactoryBuffer));

			packet->SetSourcePort(this->options.sourcePort);
			packet->SetDestinationPort(this->options.destinationPort);
			packet->SetVerificationTag(peerVerificationTag);

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
			  this->options.zeroCheksumAlternateErrorDetectionMethod !=
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE)
			{
				auto* zeroChecksumAcceptableParameter =
				  chunk->BuildParameterInPlace<ZeroChecksumAcceptableParameter>();

				zeroChecksumAcceptableParameter->SetAlternateErrorDetectionMethod(
				  this->options.zeroCheksumAlternateErrorDetectionMethod);
				zeroChecksumAcceptableParameter->Consolidate();
			}

			supportedExtensionsParameter->Consolidate();
		}

		void Socket::SendInit()
		{
			MS_TRACE();

			auto* packet    = CreatePacket();
			auto* initChunk = packet->BuildChunkInPlace<InitChunk>();

			initChunk->SetInitiateTag(this->preTcb.myVerificationTag);
			initChunk->SetAdvertisedReceiverWindowCredit(this->options.myAdvertisedReceiverWindowCredit);
			initChunk->SetNumberOfOutboundStreams(this->options.maxOutboundStreams);
			initChunk->SetNumberOfInboundStreams(this->options.maxInboundStreams);
			initChunk->SetInitialTsn(this->preTcb.myInitialTsn);

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
				auto currentStateStringView = Socket::State2String(this->state);
				std::ostringstream unexpectedStatesOss;
				bool firstUnexpectedState = true;

				// NOTE: Using fold expression operator.
				((unexpectedStatesOss << (firstUnexpectedState ? "" : ", ")
				                      << Socket::State2String(unexpectedStates),
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
	} // namespace SCTP
} // namespace RTC
