#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/SctpOptions.hpp"
#include "RTC/SCTP/SocketDeferredListener.hpp"
#include "RTC/SCTP/SocketListener.hpp"
#include "RTC/SCTP/SocketMetrics.hpp"
#include "RTC/SCTP/TransmissionControlBlock.hpp"
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
			/**
			 * Internal SCTP association state. This is different from the public SCTP
			 * Socket state (`SCTP::Types::SocketState`).
			 */
			enum class AssociationState
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

			static constexpr std::string_view AssociationStateToString(AssociationState associationState)
			{
				// NOTE: We cannot use MS_TRACE() here because clang in Linux will
				// complain about "read of non-constexpr variable 'configuration' is not
				// allowed in a constant expression".

				switch (associationState)
				{
					case Socket::AssociationState::CLOSED:
					{
						return "CLOSED";
					}

					case Socket::AssociationState::COOKIE_WAIT:
					{
						return "COOKIE_WAIT";
					}

					case Socket::AssociationState::COOKIE_ECHOED:
					{
						return "COOKIE_ECHOED";
					}

					case Socket::AssociationState::ESTABLISHED:
					{
						return "ESTABLISHED";
					}

					case Socket::AssociationState::SHUTDOWN_PENDING:
					{
						return "SHUTDOWN_PENDING";
					}

					case Socket::AssociationState::SHUTDOWN_SENT:
					{
						return "SHUTDOWN_SENT";
					}

					case Socket::AssociationState::SHUTDOWN_RECEIVED:
					{
						return "SHUTDOWN_RECEIVED";
					}

					case Socket::AssociationState::SHUTDOWN_ACK_SENT:
					{
						return "SHUTDOWN_ACK_SENT";
					}
				}
			}

			/**
			 * Struct holding local verification tag and initial TSN between having
			 * sent the INIT Chunk until the connection is established (there is no
			 * TCB in between).
			 *
			 * @remarks
			 * - This is how dcSCTP does, despite RFC 9260 states that the TCB should
			 *   also be created when an INIT Chunk is sent.
			 */
			struct PreTransmissionControlBlock
			{
				uint32_t localVerificationTag{ 0 };
				uint32_t localInitialTsn{ 0 };
			};

		public:
			explicit Socket(const SctpOptions& sctpOptions, SocketListener& listener);

			~Socket() override;

			void Dump(int indentation = 0) const;

			Types::SocketState GetState() const;

			/**
			 * Closes the connection non-gracefully. Will send ABORT if the connection
			 * is not already closed. No callbacks will be made after Close() has
			 * returned.
			 */
			void Close();

			/**
			 * Initiate the SCTP association with the remote peer. It sends an INIT
			 * Chunk.
			 *
			 * @remarks
			 * - The SCTP association must be in Closed state.
			 */
			void Connect();

			/**
			 * Receive a Packet received from the peer.
			 */
			void ReceivePacket(const Packet* receivedPacket);

		private:
			void InternalClose(Types::ErrorKind errorKind, std::string_view& message);

			void SetAssociationState(AssociationState associationState, const std::string_view& message);

			void AddCapabilitiesParametersToInitOrInitAckChunk(Chunk* chunk) const;

			void CreateTransmissionControlBlock(
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t localAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			std::unique_ptr<Packet> CreatePacket() const;

			std::unique_ptr<Packet> CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			/**
			 * Notify the parent about a Packet to be sent to the peer and return a
			 * boolean indicating whether the Packet was sent or not.
			 *
			 * This method also writes the Packet checksum field depending on the value
			 * of `writeChecksum`. If it's explicitly set then it's honored. Otherwise
			 * the checksum field is written based on whether Zero Checksum has been
			 * negotiated or not.
			 *
			 * @remarks
			 * - This method does not delete the given `packet`. The caller must do it
			 *   after invoking this method.
			 */
			bool SendPacket(Packet* packet, std::optional<bool> writeChecksum = std::nullopt);

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

			void OnT1InitTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnT1CookieTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnT2ShutdownTimer(uint64_t& baseTimeoutMs, bool& stop);

			template<typename... AssociationStates>
			void AssertAssociationState(AssociationStates... expectedAssociationStates) const;

			template<typename... AssociationStates>
			void AssertNotAssociatonState(AssociationStates... unexpectedAssociationStates) const;

			void AssertAssociationStateIsConsistent() const;

			void AssertHasTcb() const;

			/* Pure virtual methods inherited from BackoffTimerHandle::Listener. */
		public:
			void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			// SCTP options given in the constructor.
			const SctpOptions sctpOptions;
			// Listener. It's not a SocketListener but a SocketDeferredListener which
			// inherits from SocketListener.
			SocketDeferredListener listener;
			// SCTP association state.
			AssociationState associationState{ AssociationState::CLOSED };
			// Metrics.
			SocketMetrics metrics{};
			// The actual send queue implementation. As data can be sent on a Socket
			// before the connection is established, this component is not in the TCB.
			// TODO: Implement this class.
			// RRSendQueue sendQueue;
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
