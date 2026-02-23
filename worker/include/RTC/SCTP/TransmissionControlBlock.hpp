#ifndef MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP
#define MS_RTC_SCTP_TRANSMISSION_CONTROL_BLOCK_HPP

#include "common.hpp"
#include "RTC/SCTP/NegotiatedCapabilities.hpp"

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
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

			~TransmissionControlBlock();

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

		private:
			uint32_t localVerificationTag{ 0 };
			uint32_t remoteVerificationTag{ 0 };
			uint32_t localInitialTsn{ 0 };
			uint32_t remoteInitialTsn{ 0 };
			uint32_t remoteAdvertisedReceiverWindowCredit{ 0 };
			uint64_t tieTag{ 0 };
			NegotiatedCapabilities negotiatedCapabilities;
		};
	} // namespace SCTP
} // namespace RTC

#endif
