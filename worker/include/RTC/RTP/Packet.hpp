#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "FBS/rtpPacket.h"
#include "RTC/Serializable.hpp"
#include <flatbuffers/flatbuffers.h>
#include <array>
#include <map>

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
		public:
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

		private:
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
				uint8_t value[];
			};

		public:
			/**
			 * One-Byte and Two-Bytes Extension types.
			 *
			 * @see RFC 8285.
			 */
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
				uint8_t value[];
			};

		private:
			struct TwoBytesExtension
			{
				uint8_t id;
				uint8_t len;
				uint8_t value[];
			};

		public:
			/**
			 * Struct for setting and replacing Extensions.
			 */
			struct AddedExtension
			{
				AddedExtension(uint8_t id, uint8_t len, uint8_t* value) : id(id), len(len), value(value) {};

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
			 * Whether the Packet has One-Byte or Two-Bytes Extensions.
			 *
			 * @see RFC 8285.
			 */
			bool HasExtensions() const
			{
				return HasOneByteExtensions() || HasTwoBytesExtensions();
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
			 * Whether the One-Byte or Two-Bytes Extension with given `id` exists in
			 * the Packet.
			 *
			 * @see RFC 8285.
			 *
			 * @remarks
			 * - If the length of the Extension value is 0 this method returns `true`.
			 */
			bool HasExtension(uint8_t id) const
			{
				if (id == 0)
				{
					return false;
				}
				else if (HasOneByteExtensions())
				{
					if (id > 14)
					{
						return false;
					}

					// `-1` because we have 14 elements total 0..13 and `id` is in the
					// range 1..14.
					auto offset = this->oneByteExtensions[id - 1];

					return offset != -1;
				}
				else if (HasTwoBytesExtensions())
				{
					return this->twoBytesExtensions.find(id) != this->twoBytesExtensions.end();
				}
				else
				{
					return false;
				}
			}

			/**
			 * Get a pointer to the value of the the One-Byte or Two-Bytes Extension
			 * with given `id` and set its value length into given `len`.
			 *
			 * @see RFC 8285.
			 */
			uint8_t* GetExtension(uint8_t id, uint8_t& len) const
			{
				len = 0;

				if (id == 0)
				{
					return nullptr;
				}
				else if (HasOneByteExtensions())
				{
					if (id > 14)
					{
						return nullptr;
					}

					// `-1` because we have 14 elements total 0..13 and `id` is in the
					// range 1..14.
					auto offset = this->oneByteExtensions[id - 1];

					if (offset == -1)
					{
						return nullptr;
					}

					auto* extension = reinterpret_cast<OneByteExtension*>(GetHeaderExtensionValue() + offset);

					// In One-Byte Extensions value length 0 means 1.
					len = extension->len + 1;

					return extension->value;
				}
				else if (HasTwoBytesExtensions())
				{
					auto it = this->twoBytesExtensions.find(id);

					if (it == this->twoBytesExtensions.end())
					{
						return nullptr;
					}

					auto offset = it->second;

					auto* extension = reinterpret_cast<TwoBytesExtension*>(GetHeaderExtensionValue() + offset);

					len = extension->len;

					return extension->value;
				}
				else
				{
					return nullptr;
				}
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
			 * Number of entries in the CSRC list.
			 *
			 * @remarks
			 * - This method doesn't validate whether there is indeed space for the
			 *   announced CSRC list.
			 * - This method is guaranteed to return valid value once @ref Validate()
			 *   was succesfully called.
			 */
			size_t GetCsrcCount() const
			{
				return GetFixedHeaderPointer()->csrcCount * sizeof(GetFixedHeaderPointer()->ssrc);
			}

			/**
			 * Pointer to the location where Extension Header is supposed to begin.
			 */
			HeaderExtension* GetHeaderExtensionPointer() const
			{
				return reinterpret_cast<HeaderExtension*>(GetCsrcsPointer() + GetCsrcCount());
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
			bool Validate();

			/**
			 * Parses Extensions. Returns `true` if they are valid.
			 *
			 * @see RFC 8285.
			 */
			bool ParseExtensions();

		private:
			// Array of One Byte Extensions. Index is the id - 1 of the Extension,
			// each entry is the offset (in bytes) from the beginning of the Header
			// Extension value to the beginning of the Extension.
			std::array<ssize_t, 14> oneByteExtensions{ -1, -1, -1, -1, -1, -1, -1,
				                                         -1, -1, -1, -1, -1, -1, -1 };
			// Map of Two Bytes Extensions. Key is the id 1 of the Extension,
			// each entry is the offset (in bytes) from the beginning of the Header
			// Extension value to the beginning of the Extension.
			std::map<uint8_t, ssize_t> twoBytesExtensions;
		};
	} // namespace RTP
} // namespace RTC

#endif
