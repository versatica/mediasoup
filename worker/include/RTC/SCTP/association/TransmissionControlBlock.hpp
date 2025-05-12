#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The Transmission Control Block (TCB) represents an SCTP connection with
		 * a peer and holds all its state.
		 *
		 * @see https://datatracker.ietf.org/doc/html/rfc9260#name-recommended-transmission-co
		 */
		class TransmissionControlBlock
		{
		public:
			TransmissionControlBlock(
			  uint32_t myVerificationTag,
			  uint32_t peerVerificationTag,
			  uint32_t myInitialTsn,
			  uint32_t peerInitialTsn,
			  uint32_t myAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			~TransmissionControlBlock();

			void Dump(int indentation = 0) const;

			/**
			 * The value of the Initiate Tag field we put in our INIT or INIT_ACK
			 * Chunk. Packets sent by the remote peer must include this value in
			 * their Verification Tag field.
			 */
			uint32_t GetMyVerificationTag() const
			{
				return this->myVerificationTag;
			}

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT_ACK Chunk. Packets sent by us to the peer must include this value
			 * in their Verification Tag field.
			 */
			uint32_t GetPeerVerificationTag() const
			{
				return this->peerVerificationTag;
			}

			/**
			 * The value of the Initial TSN field we put in our INIT or INIT_ACK
			 * Chunk.
			 */
			uint32_t GetMyInitialTsn() const
			{
				return this->myInitialTsn;
			}

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT_ACK Chunk.
			 */
			uint32_t GetPeerInitialTsn() const
			{
				return this->peerInitialTsn;
			}

			/**
			 * The value of the Advertised Receiver Window Credit field we put in our
			 * INIT or INIT_ACK Chunk.
			 */
			uint32_t GetMyAdvertisedReceiverWindowCredit() const
			{
				return this->myAdvertisedReceiverWindowCredit;
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

		private:
			uint32_t myVerificationTag{ 0 };
			uint32_t peerVerificationTag{ 0 };
			uint32_t myInitialTsn{ 0 };
			uint32_t peerInitialTsn{ 0 };
			uint32_t myAdvertisedReceiverWindowCredit{ 0 };
			uint64_t tieTag{ 0 };
			const NegotiatedCapabilities negotiatedCapabilities;
		};
	} // namespace SCTP
} // namespace RTC

#endif
