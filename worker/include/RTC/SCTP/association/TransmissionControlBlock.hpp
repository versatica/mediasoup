#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "RTC/SCTP/association/AssociationListenerDeferrer.hpp"
#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "RTC/SCTP/association/TransmissionControlBlockContextInterface.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/rx/DataTracker.hpp"
#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include "RTC/SCTP/tx/RetransmissionErrorCounter.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include "SharedInterface.hpp"
#include <string_view>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The Transmission Control Block (TCB) represents an SCTP connection with
		 * a peer and holds all its state.
		 *
		 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-14
		 */
		class TransmissionControlBlock : public TransmissionControlBlockContextInterface,
		                                 public RetransmissionQueue::Listener,
		                                 public BackoffTimerHandleInterface::Listener
		{
		public:
			TransmissionControlBlock(
			  AssociationListenerDeferrer& associationListenerDeferrer,
			  const SctpOptions& sctpOptions,
			  SharedInterface* shared,
			  SendQueueInterface& sendQueue,
			  PacketSender& packetSender,
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities,
			  size_t maxPacketLength,
			  std::function<bool()> isAssociationEstablished);

			~TransmissionControlBlock() override;

		public:
			void Dump(int indentation = 0) const;

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			bool IsAssociationEstablished() const override
			{
				return this->isAssociationEstablished();
			}

			/**
			 * The value of the Initiate Tag field we put in our INIT or INIT-ACK
			 * chunk. Packets sent by the remote peer must include this value in
			 * their Verification Tag field.
			 */
			uint32_t GetLocalVerificationTag() const
			{
				return this->localVerificationTag;
			}

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT-ACK chunk. Packets sent by us to the peer must include this value
			 * in their Verification Tag field.
			 */
			uint32_t GetRemoteVerificationTag() const
			{
				return this->remoteVerificationTag;
			}

			/**
			 * The value of the Initial TSN field we put in our INIT or INIT-ACK
			 * chunk.
			 *
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			uint32_t GetLocalInitialTsn() const override
			{
				return this->localInitialTsn;
			}

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT-ACK chunk.
			 *
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			uint32_t GetRemoteInitialTsn() const override
			{
				return this->remoteInitialTsn;
			}

			/**
			 * The value of the Advertised Receiver Window Credit field we put in our
			 * INIT or INIT-ACK chunk.
			 */
			uint32_t GetRemoteAdvertisedReceiverWindowCredit() const
			{
				return this->remoteAdvertisedReceiverWindowCredit;
			}

			/**
			 * Tie-Tag used as a nonce when connecting.
			 */
			uint64_t GetTieTag() const
			{
				return this->tieTag;
			}

			/**
			 * Negotiated association capabilities.
			 */
			const NegotiatedCapabilities& GetNegotiatedCapabilities() const
			{
				return this->negotiatedCapabilities;
			}

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			void ObserveRttMs(uint64_t rttMs) override;

			size_t GetCwnd() const
			{
				return this->retransmissionQueue.GetCwnd();
			}

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			uint64_t GetCurrentRtoMs() const override
			{
				return this->rto.GetRtoMs();
			}

			uint64_t GetCurrentSrttMs() const
			{
				return this->rto.GetSrttMs();
			}

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			std::unique_ptr<Packet> CreatePacket() const override;

			std::unique_ptr<Packet> CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			bool SendPacket(Packet* packet) override;

			DataTracker& GetDataTracker()
			{
				return this->dataTracker;
			}

			ReassemblyQueue& GetReassemblyQueue()
			{
				return this->reassemblyQueue;
			}

			RetransmissionQueue& GetRetransmissionQueue()
			{
				return this->retransmissionQueue;
			}

			StreamResetHandler& GetStreamResetHandler()
			{
				return this->streamResetHandler;
			}

			HeartbeatHandler& GetHeartbeatHandler()
			{
				return this->heartbeatHandler;
			}

			/**
			 * Will be set while the association is in COOKIE_ECHOED state. In this
			 * state, there can only be a single packet outstanding, and it must
			 * contain the COOKIE-ECHO chunk as the first chunk in that packet, until
			 * the COOKIE-ACK has been received, which will make the socket call
			 * `ClearRemoteStateCookie()`.
			 */
			void SetRemoteStateCookie(std::vector<uint8_t> remoteStateCookie);

			/**
			 * Called when the COOKIE-ACK chunk has been received, to allow further
			 * packets to be sent.
			 */
			void ClearRemoteStateCookie();

			bool HasRemoteStateCookie() const
			{
				return this->remoteStateCookie.has_value();
			}

			/**
			 * Sends a SACK chunk, if there is a need to.
			 */
			void MaySendSackChunk();

			/**
			 * May add a FORWARD-TSN or I-FORWARD-TSN chunk to the given packet if it
			 * is needed and allowed (rate-limited).
			 */
			void MayAddForwardTsnChunk(Packet* packet, uint64_t nowMs);

			void MaySendFastRetransmit();

			/**
			 * Create and fill packets with control and DATA/I-DATA chunks, and sends
			 * them as much as can be allowed by the congestion control algorithm.
			 *
			 * @remarks
			 * - If `this->remoteStateCookie` is present, then only one packet will be
			 *   sent, with this chunk as the first chunk.
			 * - Cannot pass `addCookieAckChunk=true` if `this->remoteStateCookie` is
			 *   present (will throw).
			 */
			void SendBufferedPackets(uint64_t nowMs, bool addCookieAckChunk = false);

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			bool IncrementTxErrorCounter(std::string_view reason) override
			{
				return this->txErrorCounter.Increment(reason);
			}

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			void ClearTxErrorCounter() override
			{
				this->txErrorCounter.Clear();
			}

			/**
			 * @remarks
			 * - Implements TransmissionControlBlockContextInterface.
			 */
			bool HasTooManyTxErrors() const override
			{
				return this->txErrorCounter.IsExhausted();
			}

		private:
			void OnT3RtxTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnDelayedAckTimer(uint64_t& baseTimeoutMs, bool& stop);

			/* Pure virtual methods inherited from RetransmissionQueue::Listener. */
		public:
			void OnRetransmissionQueueNewRttMs(uint64_t newRttMs) override;
			void OnRetransmissionQueueClearRetransmissionCounter() override;
			;

			/* Pure virtual methods inherited from BackoffTimerHandleInterface::Listener. */
		public:
			void OnBackoffTimer(
			  BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListenerDeferrer& associationListenerDeferrer;
			const SctpOptions sctpOptions;
			SharedInterface* shared;
			PacketSender& packetSender;
			uint32_t localVerificationTag;
			uint32_t remoteVerificationTag;
			uint32_t localInitialTsn;
			uint32_t remoteInitialTsn;
			uint32_t remoteAdvertisedReceiverWindowCredit;
			// Nonce, used to detect reconnections.
			uint64_t tieTag;
			NegotiatedCapabilities negotiatedCapabilities;
			// Max SCTP packet length.
			const size_t maxPacketLength;
			std::function<bool()> isAssociationEstablished;
			// The data retransmission timer.
			const std::unique_ptr<BackoffTimerHandleInterface> t3RtxTimer;
			// Delayed ack timer, which triggers when acks should be sent (when
			// delayed).
			const std::unique_ptr<BackoffTimerHandleInterface> delayedAckTimer;
			RetransmissionTimeout rto;
			RetransmissionErrorCounter txErrorCounter;
			DataTracker dataTracker;
			ReassemblyQueue reassemblyQueue;
			RetransmissionQueue retransmissionQueue;
			StreamResetHandler streamResetHandler;
			HeartbeatHandler heartbeatHandler;
			// Rate limiting of FORWARD-TSN. Next can be sent at or after this
			// timestamp.
			uint64_t limitForwardTsnUntilMs{ 0 };
			// Only valid when state is State::COOKIE_ECHOED. In this state, the
			// association must wait for COOKIE-ACK to continue sending any packets (not
			// including a COOKIE-ECHO). So if this state cookie is present, the
			// `SendBufferedChunks()` method will always only send one packet, with
			// a CookieEchoChunk containing this cookie as the first chunk in the packet.
			std::optional<std::vector<uint8_t>> remoteStateCookie;
		};
	} // namespace SCTP
} // namespace RTC

#endif
