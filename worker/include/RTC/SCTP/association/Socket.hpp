#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/SocketOptions.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include <string>
#include <string_view>

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
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				virtual void OnSocketSendSctpPacket(const Socket* socket, Packet* packet) const = 0;
			};

		public:
			/**
			 * SCTP association state.
			 */
			enum class State
			{
				CLOSED,
				COOKIE_WAIT,
				// NOTE: TCB is valid in these states:
				COOKIE_ECHOED,
				ESTABLISHED,
				SHUTDOWN_PENDING,
				SHUTDOWN_SENT,
				SHUTDOWN_RECEIVED,
				SHUTDOWN_ACK_SENT,
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
			static constexpr std::string_view State2String(State state);

		public:
			explicit Socket(SocketOptions options, Listener* listener);

			~Socket();

			void Dump(int indentation = 0) const;

			/**
			 * Initiate the SCTP association with the remote peer. It sends an INIT
			 * Chunk.
			 *
			 * @remarks
			 * The Socket must be in Closed state.
			 */
			void Associate();

		private:
			void SetState(State state, const std::string& reason);

			Packet* CreatePacket() const;

			Packet* CreatePacketWithPeerVerificationTag(uint32_t peerVerificationTag) const;

			void AddCapabilitiesParametersToChunk(Chunk* chunk) const;

			void SendInit();

			template<typename... States>
			void AssertState(States... expectedStates) const;

			template<typename... States>
			void AssertNotState(States... unexpectedStates) const;

		private:
			// Socket options given in th econstructor.
			const SocketOptions options;
			// Listener.
			const Listener* listener{ nullptr };
			// SCTP association state.
			State state{ State::CLOSED };
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
