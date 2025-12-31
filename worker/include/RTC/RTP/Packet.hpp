#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "FBS/rtpPacket.h"
#include "RTC/Serializable.hpp"
#include <flatbuffers/flatbuffers.h>
#include <cstdint>

namespace RTC
{
	namespace RTP
	{
		/**
		 * RTP Packet.
		 *
		 * @see RFC 3550.
		 */

		class Packet : public Serializable
		{
			/**
			 * RTP Fixed Header.
			 *
			 * @see RFC 3550.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                           timestamp                           |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |           synchronization source (SSRC) identifier            |
			 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
			 * |            contributing source (CSRC) identifiers             |
			 * |                             ....                              |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

		public:
			struct FixedHeader
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t csrcCount : 4;
				uint8_t extension : 1;
				uint8_t padding : 1;
				uint8_t version : 2;
				uint8_t payloadType : 7;
				uint8_t marker : 1;
#elif defined(MS_BIG_ENDIAN)
				uint8_t version : 2;
				uint8_t padding : 1;
				uint8_t extension : 1;
				uint8_t csrcCount : 4;
				uint8_t marker : 1;
				uint8_t payloadType : 7;
#endif
				uint16_t sequenceNumber;
				uint32_t timestamp;
				uint32_t ssrc;
			};

			/**
			 * RTP Header Extension.
			 *
			 * @see RFC 3350.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |      defined by profile       |           length              |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                        header extension                       |
			 * |                             ....                              |
			 */

		private:
			struct HeaderExtension
			{
				/**
				 * Defined by profile.
				 */
				uint16_t id;
				/**
				 * Number of 32-bit words in the extension, excluding the id & length
				 * four-octet.
				 */
				uint16_t length;
				uint8_t value[1];
			};

			/**
			 * One-Byte and Two-Bytes Extensions.
			 *
			 * @see RFC 8285.
			 */

		public:
			enum class ExtensionsType : uint8_t
			{
				OneByte  = 1,
				TwoBytes = 2
			};

		private:
			struct OneByteExtension
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t len : 4;
				uint8_t id : 4;
#elif defined(MS_BIG_ENDIAN)
				uint8_t id : 4;
				uint8_t len : 4;
#endif
				uint8_t value[1];
			};

		private:
			/* Struct for Two-Bytes extension. */
			struct TwoBytesExtension
			{
				uint8_t id;
				uint8_t len;
				uint8_t value[1];
			};

		public:
			/**
			 * Struct for setting and replacing header extensions.
			 */
			struct GenericExtension
			{
				GenericExtension(uint8_t id, uint8_t len, uint8_t* value)
				  : id(id), len(len), value(value) {};

				uint8_t id;
				uint8_t len;
				uint8_t* value;
			};

		public:
			/**
			 * Minimum size (in bytes) of the RTP Fixed Header (without CSRC fields).
			 */
			static const size_t FixedHeaderMinSize{ 12 };

			/**
			 * Whether given buffer could be a valid RTP Packet.
			 *
			 * @remarks
			 * - Before calling this static method, the caller should verify whether
			 *   the given buffer is a RTCP packet.
			 */
			static bool IsRtp(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a RTP Packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the Packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Creates a RTP Packet.
			 */
			static Packet* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Constructor is private because we only want to create Packet instances
			 * via Parse() and Factory().
			 */
			Packet(uint8_t* buffer, size_t bufferLength);

		public:
			~Packet() override;

			void Dump(int indentation = 0) const final;

			void Serialize(uint8_t* buffer, size_t bufferLength) final;

			Packet* Clone(uint8_t* buffer, size_t bufferLength) const final;

			flatbuffers::Offset<FBS::RtpPacket::Dump> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

			uint8_t GetVersion() const
			{
				return GetFixedHeaderPointer()->version;
			}

			uint8_t GetPayloadType() const
			{
				return GetFixedHeaderPointer()->payloadType;
			}

			void SetPayloadType(uint8_t payloadType);

			bool HasMarker() const
			{
				return GetFixedHeaderPointer()->marker;
			}

			void SetMarker(bool marker);

			uint16_t GetSequenceNumber() const
			{
				return ntohs(GetFixedHeaderPointer()->sequenceNumber);
			}

			void SetSequenceNumber(uint16_t seq);

			uint32_t GetTimestamp() const
			{
				return ntohl(GetFixedHeaderPointer()->timestamp);
			}

			void SetTimestamp(uint32_t timestamp);

			uint32_t GetSsrc() const
			{
				return ntohl(GetFixedHeaderPointer()->ssrc);
			}

			void SetSsrc(uint32_t ssrc);

			bool HasCsrcs() const
			{
				return (GetFixedHeaderPointer()->csrcCount != 0);
			}

			bool HasHeaderExtension() const
			{
				return (GetFixedHeaderPointer()->extension);
			}

			/**
			 * Get the Extension Header id or 0 if there isn't.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint16_t GetHeaderExtensionId() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return ntohs(GetHeaderExtensionPointer()->id);
			}

			/**
			 * Pointer to the Header Extension value or `nullptr` if there is no
			 * Header Extension or its has no value.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint8_t* GetHeaderExtensionValue() const
			{
				auto headerExtensionValueLength = GetHeaderExtensionValueLength();

				if (headerExtensionValueLength == 0)
				{
					return nullptr;
				}

				return GetHeaderExtensionPointer()->value;
			}

			/**
			 * Length of the Header Extension value (excluding the id & length
			 * four-octet).
			 */
			size_t GetHeaderExtensionValueLength() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return static_cast<size_t>(ntohs(GetHeaderExtensionPointer()->length) * 4);
			}

			/**
			 * Whether the Packet has One-Byte Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasOneByteExtensions() const
			{
				return GetHeaderExtensionId() == 0xBEDE;
			}

			/**
			 * Whether the Packet has Two-Bytes Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasTwoBytesExtensions() const
			{
				return (GetHeaderExtensionId() & 0b1111111111110000) == 0b0001000000000000;
			}

			/**
			 * Whether this Packet has payload.
			 */
			bool HasPayload() const
			{
				return GetPayloadLength() > 0;
			}

			/**
			 * Pointer to the beginning of the payload (if any).
			 *
			 * @remarks
			 * - This method doens't take into account padding, so in a padding-only
			 *   Packet this method returns `nullptr`.
			 */
			uint8_t* GetPayload() const
			{
				return HasPayload() ? GetPayloadPointer() : nullptr;
			}

			/**
			 * Length of the payload excluding padding bytes.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetPayloadLength() const
			{
				const size_t availablePayloadAndPaddingLength =
				  GetLength() - (GetPayloadPointer() - GetBuffer());
				const auto paddingLength = GetPaddingLength();

				// If there is announced padding, compute effective payload length
				// without padding.
				if (availablePayloadAndPaddingLength >= paddingLength)
				{
					return availablePayloadAndPaddingLength - paddingLength;
				}
				// If there are more announced padding bytes than the available length
				// for payload and padding, return 0.
				else
				{
					return 0;
				}
			}

			/**
			 * Whether this Packet has padding.
			 */
			bool HasPadding() const
			{
				return (GetFixedHeaderPointer()->padding);
			}

			/**
			 * Length of the padding.
			 *
			 * @remarks
			 * - This method doesn't validate whether the padding length announced in
			 *   the last byte of the Packet is valid.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			uint8_t GetPaddingLength() const
			{
				if (!HasPadding())
				{
					return 0;
				}

				return GetBuffer()[GetLength() - 1];
			}

		private:
			/**
			 * @remarks
			 * - Returns FixedHeader* instead of const FixedHeader* since we may want
			 *   to modify its fields.
			 */
			FixedHeader* GetFixedHeaderPointer() const
			{
				return reinterpret_cast<FixedHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			/**
			 * Pointer to the location where the CSRC list is supposed to begin.
			 */
			uint8_t* GetCsrcsPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + Packet::FixedHeaderMinSize;
			}

			/**
			 * Length of the CSRC list.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced CSRC list.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetCsrcsLength() const
			{
				return GetFixedHeaderPointer()->csrcCount * sizeof(GetFixedHeaderPointer()->ssrc);
			}

			/**
			 * Pointer to the location where Extension Header is supposed to begin.
			 */
			HeaderExtension* GetHeaderExtensionPointer() const
			{
				return reinterpret_cast<HeaderExtension*>(GetCsrcsPointer() + GetCsrcsLength());
			}

			/**
			 * Length of the Header Extension including the id & length four-octet.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced Header Extension.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetHeaderExtensionTotalLength() const
			{
				if (!HasHeaderExtension())
				{
					return 0;
				}

				return 4 + static_cast<size_t>(ntohs(GetHeaderExtensionPointer()->length) * 4);
			}

			/**
			 * Pointer to the location where the payload is supposed to begin.
			 */
			uint8_t* GetPayloadPointer() const
			{
				return reinterpret_cast<uint8_t*>(GetHeaderExtensionPointer()) +
				       GetHeaderExtensionTotalLength();
			}

			/**
			 * Validates whether the Packet is valid.
			 */
			bool Validate() const;
		};
	} // namespace RTP
} // namespace RTC

#endif
