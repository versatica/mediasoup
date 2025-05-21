#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/SocketMetrics.hpp"
#include "RTC/SCTP/association/SocketOptions.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
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
			void ReceivePacket(const Packet* receivedPacket);

		private:
			void SetState(State state, const std::string& reason);

			void AddCapabilitiesParametersToInitOrInitAckChunk(Chunk* chunk) const;

			void CreateTransmissionControlBlock(
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t localAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			Packet* CreatePacket() const;

			Packet* CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			/**
			 * Notify the parent about a Packet to be sent to the peer.
			 *
			 * This method also writes the Packet checksum field depending on the value
			 * of `writeChecksum`. If it's explicitly set then it's honored. Otherwise
			 * the checksum field is written based on whether Zero Checksum has been
			 * negotiated or not.
			 *
			 * @remarks
			 * This method does not delete the given `packet`. The caller must do it
			 * after invoking this method.
			 */
			void SendPacket(Packet* packet, std::optional<bool> writeChecksum = std::nullopt);

			void SendInitChunk();

			void SendShutdownAckChunk();

			bool ValidateReceivedPacket(const Packet* receivedPacket);

			bool ProcessReceivedChunk(const Packet* receivedPacket, const Chunk* receivedChunk);

			void ProcessReceivedDataChunk(const Packet* receivedPacket, const DataChunk* receivedDataChunk);

			void ProcessReceivedInitChunk(const Packet* receivedPacket, const InitChunk* receivedInitChunk);

			void ProcessReceivedInitAckChunk(
			  const Packet* receivedPacket, const InitAckChunk* receivedInitAckChunk);

			void ProcessReceivedSackChunk(const Packet* receivedPacket, const SackChunk* receivedSackChunk);

			void ProcessReceivedHeartbeatRequestChunk(
			  const Packet* receivedPacket, const HeartbeatRequestChunk* receivedHeartbeatRequestChunk);

			bool ProcessReceivedUnknownChunk(
			  const Packet* receivedPacket, const UnknownChunk* receivedUnknownChunk);

			void OnT1InitTimer(uint64_t& baseTimeout, bool& stop);

			void OnT1CookieTimer(uint64_t& baseTimeout, bool& stop);

			void OnT2ShutdownTimer(uint64_t& baseTimeout, bool& stop);

			template<typename... States>
			void AssertState(States... expectedStates) const;

			template<typename... States>
			void AssertNotState(States... unexpectedStates) const;

			void AssertHasTcb() const;

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
