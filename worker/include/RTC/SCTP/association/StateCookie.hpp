#ifndef MS_RTC_SCTP_STATE_COOKIE_HPP
#define MS_RTC_SCTP_STATE_COOKIE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/Serializable.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * This is the State Cookie we generate and put into a State Cookie
		 * Parameter when we send INIT_ACK Chunk to the remote peer.
		 *
		 * The syntax we use is as follows. Note that we use a fixed length of
		 * StateCookieLength bytes.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         Magic Value 1                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      My Verification Tag                      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     Peer Verification Tag                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                        My Initial TSN                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       Peer Initial TSN                        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        My Advertised Receiver Window Credit (a_rwnd)          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                            Tie-Tag                            |
		 * |                                                               |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                     Negotiated Capabilities                   /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * Negotiated Capabilities are serialized as follows:
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   (Reserved)  |       |D|C|B|A|        Magic Value 2          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      Max Outbound Streams     |       Max Inbound Streams     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Flag A (partialReliability): Partial Reliability Extension.
		 * - Flag B (messageInterleaving): Stream Schedulers and User Message
		 *   Interleaving (I-DATA).
		 * - Flag C (reconfig): Stream Reconfiguration.
		 * - Flag D (zeroChecksum): Zero Checksum.
		 */
		class StateCookie : public Serializable
		{
		public:
			static constexpr size_t StateCookieLength{ 40 };
			// Magic value we prefix the State Cookie with.
			static constexpr uint32_t MagicValue1{ 0xF109ABE4 };
			// Magic value used within the Negotiated Capabilities block.
			static constexpr uint16_t MagicValue2{ 0xAD81 };

		public:
			/**
			 * Parse a StateCookie.
			 *
			 * @remarks
			 * `bufferLength` must be the exact length of the State Cookie.
			 */
			static StateCookie* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a StateCookie.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the real length of the State
			 * Cookie.
			 */
			static StateCookie* Factory(
			  uint8_t* buffer,
			  size_t bufferLength,
			  uint32_t myVerificationTag,
			  uint32_t peerVerificationTag,
			  uint32_t myInitialTsn,
			  uint32_t peerInitialTsn,
			  uint32_t myAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

		private:
			struct NegotiatedCapabilitiesField
			{
				uint8_t reserved;
#if defined(MS_LITTLE_ENDIAN)
				uint8_t bitA : 1;
				uint8_t bitB : 1;
				uint8_t bitC : 1;
				uint8_t bitD : 1;
				uint8_t unusedBits : 4;
#elif defined(MS_BIG_ENDIAN)
				uint8_t unusedBits : 4;
				uint8_t bitD : 1;
				uint8_t bitC : 1;
				uint8_t bitB : 1;
				uint8_t bitA : 1;
#endif
				uint16_t magicValue2;
				uint16_t maxOutboundStreams;
				uint16_t maxInboundStreams;
			};

		public:
			StateCookie(uint8_t* buffer, size_t bufferLength);

			~StateCookie() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual StateCookie* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			/**
			 * The value of the Initiate Tag field we put in our INIT or INIT_ACK
			 * Chunk. Packets sent by the remote peer must include this value in
			 * their Verification Tag field.
			 */
			uint32_t GetMyVerificationTag() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			/**
			 * The value of the Initiate Tag field the peer put in its INIT or
			 * INIT_ACK Chunk. Packets sent by us to the peer must include this value
			 * in their Verification Tag field.
			 */
			uint32_t GetPeerVerificationTag() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			/**
			 * The value of the Initial TSN field we put in our INIT or INIT_ACK
			 * Chunk.
			 */
			uint32_t GetMyInitialTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 12);
			}

			/**
			 * The value of the Initial TSN field the peer put in its INIT or
			 * INIT_ACK Chunk.
			 */
			uint32_t GetPeerInitialTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 16);
			}

			/**
			 * The value of the Advertised Receiver Window Credit field we put in our
			 * INIT or INIT_ACK Chunk.
			 */
			uint32_t GetMyAdvertisedReceiverWindowCredit() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 20);
			}

			/**
			 * Tie-Tag used as a nonce when connecting.
			 */
			uint64_t GetTieTag() const
			{
				return Utils::Byte::Get8Bytes(GetBuffer(), 24);
			}

			/**
			 * Negotiated association capabilities.
			 */
			NegotiatedCapabilities GetNegotiatedCapabilities() const;

		private:
			NegotiatedCapabilitiesField* GetNegotiatedCapabilitiesField() const
			{
				return reinterpret_cast<NegotiatedCapabilitiesField*>(const_cast<uint8_t*>(GetBuffer()) + 32);
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
