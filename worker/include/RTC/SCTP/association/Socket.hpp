#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/SocketOptions.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The SCTP Socket class represents the mediasoup side of an SCTP
		 * association with a peer.
		 *
		 * It manages all Packet and Chunk dispatching and the connection flow.
		 */
		class Socket
		{
		public:
			/**
			 * SCTP association state.
			 */
			enum class State
			{
				Closed,
				CookieWait,
				// NOTE: TCB is valid in these states:
				CookieEchoed,
				Established,
				ShutdownPending,
				ShutdownSent,
				ShutdownReceived,
				ShutdownAckSent,
			};

			/**
			 * Struct holding local verification tag and initial TSN between having
			 * sent the INIT Chunk until the connection is established (there is no
			 * TCB in between).
			 */
			struct PreTransmissionControlBlock
			{
				uint32_t myVerificationTag{ 0 };
				uint32_t myInitialTsn{ 0 };
			};

		public:
			explicit Socket(SocketOptions options);

			~Socket();

			void Dump(int indentation = 0) const;

			void Associate();

		private:
			void SetState(State state);

			Packet* CreatePacket() const;

			Packet* CreatePacketWithPeerVerificationTag(uint32_t peerVerificationTag) const;

			void AddCapabilitiesParametersToChunk(Chunk* chunk) const;

			void SendInit();

		private:
			// Socket options given in th econstructor.
			SocketOptions options;
			// SCTP association state.
			State state{ State::Closed };
			// To keep settings between sending of INIT Chunk and establishment of
			// the connection.
			PreTransmissionControlBlock preTcb;
			// Once the SCTP association is established a Transmission Control Block
			// is created (and also when we are the initiator of the association).
			std::unique_ptr<TransmissionControlBlock> tcb;
		};
	} // namespace SCTP
} // namespace RTC

#endif
