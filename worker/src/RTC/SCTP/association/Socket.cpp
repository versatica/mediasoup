#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include <limits>

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		thread_local static uint8_t FactoryBuffer[RTC::Consts::MaxSafeMtuSizeForSctp];

		/* Instance methods. */

		Socket::Socket(SocketOptions options) : options(options)
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

			if (this->state != State::Closed)
			{
				// TODO: We may want to abort here since it's a bug.
				MS_WARN_TAG(sctp, "Associate() called when state is not 'Closed'");

				return;
			}

			this->preTcb.myVerificationTag =
			  Utils::Crypto::GetRandomUInt(1, std::numeric_limits<uint32_t>::max());
			this->preTcb.myInitialTsn =
			  Utils::Crypto::GetRandomUInt(0, std::numeric_limits<uint32_t>::max());

			SendInit();

			// TODO
			// t1_init_->Start();
			// SetState(State::kCookieWait, "Connect called");
		}

		void Socket::SetState(State state)
		{
			MS_TRACE();

			if (state == this->state)
			{
				// TODO: Warn?

				return;
			}

			this->state = state;

			// TODO: Print it (we need a State2String() function).
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

			// TODO: Emit event to parent or send it somehow.

			delete packet;
		}
	} // namespace SCTP
} // namespace RTC
