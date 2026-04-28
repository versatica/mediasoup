#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "SharedInterface.hpp"
#include "RTC/SCTP/association/HeartbeatHandler.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/association/StreamResetHandler.hpp"
#include "RTC/SCTP/association/TCBContext.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionErrorCounter.hpp"
#include "RTC/SCTP/tx/RetransmissionQueue.hpp"
#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
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
		class TransmissionControlBlock : public TCBContext,
		                                 public RetransmissionQueue::Listener,
		                                 public BackoffTimerHandleInterface::Listener
		{
		public:
			TransmissionControlBlock(
			  AssociationListener& associationListener,
			  const SctpOptions& sctpOptions,
			  SharedInterface* shared,
			  // TODO: SCTP: Implement it.
			  // SendQueue& sendQueue,
			  PacketSender& packetSender,
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities,
			  std::function<bool()> isAssociationEstablished);

			~TransmissionControlBlock() override;

		public:
			void Dump(int indentation = 0) const;

			/**
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			bool IsAssociationEstablished() const override
			{
				return this->isAssociationEstablished();
			}

			/**
			 * The value of the Initiate Tag field we put in our INIT or INIT_ACK
			 * Chunk. Packets sent by the remote peer must include this value in
			 * their Verification Tag field.
			 */
			uint32_t GetLocalVerificationTag() const
			{
				return this->localVerificationTag;
			}

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT_ACK Chunk. Packets sent by us to the peer must include this value
			 * in their Verification Tag field.
			 */
			uint32_t GetRemoteVerificationTag() const
			{
				return this->remoteVerificationTag;
			}

			/**
			 * The value of the Initial TSN field we put in our INIT or INIT_ACK
			 * Chunk.
			 *
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			uint32_t GetLocalInitialTsn() const override
			{
				return this->localInitialTsn;
			}

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT_ACK Chunk.
			 *
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			uint32_t GetRemoteInitialTsn() const override
			{
				return this->remoteInitialTsn;
			}

			/**
			 * The value of the Advertised Receiver Window Credit field we put in our
			 * INIT or INIT_ACK Chunk.
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
			 * - Implements TCBContext interface.
			 */
			void ObserveRttMs(uint64_t rttMs) override;

			size_t GetCwnd() const
			{
				return this->retransmissionQueue.GetCwnd();
			}

			/**
			 * @remarks
			 * - Implements TCBContext interface.
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
			 * - Implements TCBContext interface.
			 */
			std::unique_ptr<Packet> CreatePacket() const override;

			std::unique_ptr<Packet> CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			/**
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			void Send(Packet* packet) override;

			// TODO: SCTP: Implement it.
			// DataTracker& GetDataTracker()
			// {
			// 	return this->dataTracker;
			// }

			// TODO: SCTP: Implement it.
			// ReassemblyQueue& GetReassemblyQueue()
			// {
			// 	return this->reassemblyQueue;
			// }

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
			 * Will be set while the Association is in COOKIE_ECHOED state. In this
			 * state, there can only be a single Packet outstanding, and it must
			 * contain the COOKIE_ECHO Chunk as the first Chunk in that Packet, until
			 * the COOKIE_ACK has been received, which will make the socket call
			 * `ClearRemoteStateCookie()`.
			 */
			void SetRemoteStateCookie(std::vector<uint8_t> remoteStateCookie);

			/**
			 * Called when the COOKIE_ACK Chunk has been received, to allow further
			 * Packets to be sent.
			 */
			void ClearRemoteStateCookie();

			bool HasRemoteStateCookie() const
			{
				return this->remoteStateCookie.has_value();
			}

			/**
			 * Sends a SACK Chunk, if there is a need to.
			 */
			void MaySendSackChunk();

			/**
			 * Sends a FORWARD-TSN or I-FORWARD-TSN Chunk if it is needed and allowed
			 * (rate-limited).
			 */
			void MaybeSendForwardTsnChunk(Packet* packet, uint64_t nowMs);

			void MaySendFastRetransmit();

			// TODO: SCTP: Mamy more methods.

			/**
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			bool IncrementTxErrorCounter(std::string_view reason) override
			{
				return this->txErrorCounter.Increment(reason);
			}

			/**
			 * @remarks
			 * - Implements TCBContext interface.
			 */
			void ClearTxErrorCounter() override
			{
				return this->txErrorCounter.Clear();
			}

			/**
			 * @remarks
			 * - Implements TCBContext interface.
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
			void OnTimer(BackoffTimerHandleInterface* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListener& associationListener;
			const SctpOptions sctpOptions;
			SharedInterface* shared;
			PacketSender& packetSender;
			uint32_t localVerificationTag{ 0 };
			uint32_t remoteVerificationTag{ 0 };
			uint32_t localInitialTsn{ 0 };
			uint32_t remoteInitialTsn{ 0 };
			uint32_t remoteAdvertisedReceiverWindowCredit{ 0 };
			// Nonce, used to detect reconnections.
			uint64_t tieTag{ 0 };
			NegotiatedCapabilities negotiatedCapabilities;
			std::function<bool()> isAssociationEstablished;
			// The data retransmission timer.
			const std::unique_ptr<BackoffTimerHandleInterface> t3RtxTimer;
			// Delayed ack timer, which triggers when acks should be sent (when
			// delayed).
			const std::unique_ptr<BackoffTimerHandleInterface> delayedAckTimer;
			RetransmissionTimeout rto;
			RetransmissionErrorCounter txErrorCounter;
			// TODO: SCTP: Implement.
			// DataTracker dataTracker;
			// TODO: SCTP: Implement.
			// ReassemblyQueue reassemblyQueue;
			// TODO: SCTP: Implement.
			RetransmissionQueue retransmissionQueue;
			StreamResetHandler streamResetHandler;
			HeartbeatHandler heartbeatHandler;
			// Rate limiting of FORWARD_TSN. Next can be sent at or after this
			// timestamp.
			// TODO: SCTP: Uncomment.
			uint64_t limitForwardTsnUntilMs{ 0 };
			// Only valid when state is State::COOKIE_ECHOED. In this state, the
			// Association must wait for COOKIE_ACK to continue sending any packets (not
			// including a COOKIE_ECHO). So if this state cookie is present, the
			// `SendBufferedChunks()` method will always only send one Packet, with
			// a CookieEchoChunk containing this cookie as the first Chunk in the Packet.
			std::optional<std::vector<uint8_t>> remoteStateCookie;
		};
	} // namespace SCTP
} // namespace RTC

#endif
