#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/PacketSender.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/RetransmissionErrorCounter.hpp"
#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include "handles/BackoffTimerHandle.hpp"
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
		class TransmissionControlBlock : public BackoffTimerHandle::Listener
		{
		public:
			TransmissionControlBlock(
			  AssociationListener& associationListener,
			  const SctpOptions& sctpOptions,
			  PacketSender& packetSender,
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			~TransmissionControlBlock() override;

		public:
			void Dump(int indentation = 0) const;

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
			 */
			uint32_t GetLocalInitialTsn() const
			{
				return this->localInitialTsn;
			}

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT_ACK Chunk.
			 */
			uint32_t GetRemoteInitialTsn() const
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

			void ObserveRtt(uint64_t rtt);

			uint64_t GetCurrentRtoMs() const
			{
				return this->rto.GetRtoMs();
			}

			uint64_t GetCurrentSrttMs() const
			{
				return this->rto.GetSrttMs();
			}

			std::unique_ptr<Packet> CreatePacket() const;

			std::unique_ptr<Packet> CreatePacketWithVerificationTag(uint32_t verificationTag) const;

			void SetRemoteStateCookie(std::vector<uint8_t> remoteStateCookie);

			void ClearRemoteStateCookie();

			bool HasRemoteStateCookie() const
			{
				return this->remoteStateCookie.has_value();
			}

			void MaySendSackChunk();

		private:
			void Send(Packet* packet);

			void OnT3RtxTimer(uint64_t& baseTimeoutMs, bool& stop);

			void OnDelayedAckTimer(uint64_t& baseTimeoutMs, bool& stop);

			/* Pure virtual methods inherited from BackoffTimerHandle::Listener. */
		public:
			void OnTimer(BackoffTimerHandle* backoffTimer, uint64_t& baseTimeoutMs, bool& stop) override;

		private:
			AssociationListener& associationListener;
			const SctpOptions sctpOptions;
			PacketSender& packetSender;
			uint32_t localVerificationTag{ 0 };
			uint32_t remoteVerificationTag{ 0 };
			uint32_t localInitialTsn{ 0 };
			uint32_t remoteInitialTsn{ 0 };
			uint32_t remoteAdvertisedReceiverWindowCredit{ 0 };
			// Nonce, used to detect reconnections.
			uint64_t tieTag{ 0 };
			NegotiatedCapabilities negotiatedCapabilities;
			// data retransmission timer).
			const std::unique_ptr<BackoffTimerHandle> t3RtxTimer;
			// Delayed ack timer, which triggers when acks should be sent (when
			// delayed).
			const std::unique_ptr<BackoffTimerHandle> delayedAckTimer;
			RetransmissionTimeout rto;
			RetransmissionErrorCounter txErrorCounter;
			// Rate limiting of FORWARD_TSN. Next can be sent at or after this
			// timestamp.
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
