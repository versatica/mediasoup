#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/SocketMetrics.hpp"
#include "RTC/SCTP/association/SocketOptions.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "handles/BackoffTimerHandle.hpp"
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
		class Socket : public BackoffTimerHandle::Listener
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
			 *
			 * @remarks
			 * This is how dcSCTP does, despite RFC 9260 states that the TCB should
			 * also be created when an INIT Chunk is sent.
			 */
			struct PreTransmissionControlBlock
			{
				uint32_t localVerificationTag{ 0 };
				uint32_t localInitialTsn{ 0 };
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

			/**
			 * Receive a Packet received from the peer.
			 */
			void ReceivePacket(const Packet* packet);

		private:
			void SetState(State state, const std::string& reason);

			Packet* CreatePacket() const;

			Packet* CreatePacketWithRemoteVerificationTag(uint32_t remoteVerificationTag) const;

			void AddCapabilitiesParametersToChunk(Chunk* chunk) const;

			bool ValidateReceivedPacket(const Packet* packet);

			bool ProcessReceivedChunk(const Packet* packet, const Chunk* chunk);

			void CreateTransmissionControlBlock(
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t localAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			void SendInit();

			void OnT1InitTimer(uint64_t& baseTimeout, bool& stop);

			void OnT1CookieTimer(uint64_t& baseTimeout, bool& stop);

			void OnT2ShutdownTimer(uint64_t& baseTimeout, bool& stop);

			template<typename... States>
			void AssertState(States... expectedStates) const;

			template<typename... States>
			void AssertNotState(States... unexpectedStates) const;

			/* Pure virtual methods inherited from BackoffTimerHandle::Listener. */
		public:
			void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeout, bool& stop) override;

		private:
			// Socket options given in th econstructor.
			const SocketOptions options;
			// Listener.
			const Listener* listener{ nullptr };
			// SCTP association state.
			State state{ State::CLOSED };
			// Metrics.
			SocketMetrics metrics{};
			// To keep settings between sending of INIT Chunk and establishment of
			// the connection.
			PreTransmissionControlBlock preTcb;
			// Once the SCTP association is established a Transmission Control Block
			// is created.
			std::unique_ptr<TransmissionControlBlock> tcb;
			// T1-init timer.
			const std::unique_ptr<BackoffTimerHandle> t1InitTimer;
			// T1-cookie timer.
			const std::unique_ptr<BackoffTimerHandle> t1CookieTimer;
			// T2-shutdown timer.
			const std::unique_ptr<BackoffTimerHandle> t2ShutdownTimer;
		};
	} // namespace SCTP
} // namespace RTC

#endif
