#ifndef MS_RTC_NEW_RTCP_PACKET_HPP
#define MS_RTC_NEW_RTCP_PACKET_HPP

#include "common.hpp"
#include "RTC/Serializable.hpp"
#include "Utils.hpp"
#include <ankerl/unordered_dense.h>
#include <cstdint>
#include <string>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP Packet.
		 *
		 * @see RFC 3550.
		 *
		 * @remarks
		 * - This class represents a single RTCP packet and not a compound packet.
		 */
		class Packet : public Serializable
		{
			// We need that CompoundPacket calls protected and private methods in this
			// class.
			friend class CompoundPacket;

		public:
			/**
			 * RTCP Packet Type.
			 */
			enum class PacketType : uint8_t
			{
				/**
				 * Extended Jitter Report.
				 */
				IJ = 195,
				/**
				 * RTCP Sender Report.
				 */
				SR = 200,
				/**
				 * RTCP Receiver Report.
				 */
				RR = 201,
				/**
				 * RTCP Sender Report.
				 */
				SDES = 202,
				/**
				 * RTCP BYE.
				 */
				BYE = 203,
				/**
				 * RTCP APP.
				 */
				APP = 204,
				/**
				 * RTCP Transport Layer Feedback.
				 */
				RTPFB = 205,
				/**
				 * RTCP Payload Specific Feedback.
				 */
				PSFB = 206,
				/**
				 * RTCP Extended Report.
				 */
				XR = 207,
			};

			/**
			 * RTCP Packet (RTCP Common Header + Value).
			 *
			 * @see RFC 3550.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |V=2|P|  Count  |      PT       |            Length             |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * \                                                               \
			 * /                             Value                             /
			 * \                                                               \
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *
			 * - Version (2 bits): RTP/RTCP version. Always 2.
			 * - Padding (1 bit): If set, the packet contains padding bytes at the
			 *   end that are not part of the value. The last padding byte indicates
			 *   how many padding bytes must be ignored, including itself (it will be
			 *   a multiple of four).
			 * - Count (5 bits): Field whose meaning depends on the packet type
			 *   (PT). RFC 3550 does not assign it a single name because it varies
			 *   per packet: in Sender/Receiver Report it is the reception report
			 *   block count (RC), in SDES and BYE it is the source count (SC), and
			 *   in APP it acts as an application-defined subtype. It is treated
			 *   generically as a 5-bit value specific to each packet type.
			 * - Packet type (8 bits): Identifies the RTCP packet type. Common
			 *   values: 200 (SR), 201 (RR), 202 (SDES), 203 (BYE), 204 (APP).
			 * - Length (16 bits): Length of the RTCP packet in 32-bit words minus
			 *   one, including the Common Header and any padding. Therefore the
			 *   total size in bytes is (Length + 1) * 4.
			 * - Value (variable length): Packet content, whose format depends on
			 *   the packet type (PT).
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 4 bytes.
			 * - The padding mechanism is not implemented since it's only used when
			 *   encrypting the compound packet by following the section 9.1 of RFC
			 *   3550 which absolutely nobody does (SRTCP is used instead). So
			 *   packets with Padding bit set to 1 are considered invalid and
			 *   rejected.
			 */
			struct CommonHeader
			{
#if defined(MS_LITTLE_ENDIAN)
				uint8_t count : 5;
				uint8_t padding : 1;
				uint8_t version : 2;
#elif defined(MS_BIG_ENDIAN)
				uint8_t version : 2;
				uint8_t padding : 1;
				uint8_t count : 5;
#endif
				PacketType packetType;
				uint16_t length;
			};

		public:
			/**
			 * Length (in bytes) of the RTCP Common Header.
			 */
			static constexpr size_t CommonHeaderLength{ 4 };

			/**
			 * Whether given buffer could be a valid RTCP packet.
			 */
			static bool IsRtcp(const uint8_t* buffer, size_t bufferLength);

			static const std::string& PacketTypeToString(PacketType packetType);

		protected:
			/**
			 * Whether given buffer could be a a valid RTCP packet.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real packet length.
			 * @param packetType - If given buffer is a valid packet then `packetType`
			 *   is rewritten to parsed PacketType.
			 * @param packetLength - If given buffer is a valid packet then
			 *   `packetLength` is rewritten to the computed length of the packet.
			 *
			 * @remarks
			 * - Packets with Padding bit set to 1 are not supported (see note above)
			 *   and hence this function returns false if it's set.
			 */
			static bool IsPacket(
			  const uint8_t* buffer, size_t bufferLength, PacketType& packetType, size_t& packetLength);

		private:
			static const ankerl::unordered_dense::map<PacketType, std::string> PacketType2String;

		protected:
			/**
			 * Constructor is protected because we only want to create packets
			 * instances via Parse() and Factory() in subclasses.
			 */
			Packet(uint8_t* buffer, size_t bufferLength);

		public:
			~Packet() override;

			/**
			 * Must be overridden by each subclass.
			 */
			void Dump(int indentation = 0) const override = 0;

			/**
			 * Must be overridden by each subclass.
			 */
			Packet* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			uint8_t GetVersion() const
			{
				return GetCommonHeaderPointer()->version;
			}

			PacketType GetType() const
			{
				return GetCommonHeaderPointer()->packetType;
			}

			/**
			 * False by default. UnknownPacket class overrides this method to return
			 * true instead.
			 */
			virtual bool HasUnknownType() const
			{
				return false;
			}

			/**
			 * Whether the Padding bit is set to 1.
			 *
			 * @remarks
			 * - The padding mechanism is not implemented since it's only used when
			 *   encrypting the compound packet by following the section 9.1 of RFC
			 *   3550 which absolutely nobody does (SRTCP is used instead). So
			 *   packets with Padding bit set to 1 are considered invalid and
			 *   rejected.
			 */
			bool HasPadding() const
			{
				return GetCommonHeaderPointer()->padding;
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			void DumpCommon(int indentation) const;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			/**
			 * Can be overridden by each subclass.
			 */
			virtual Packet* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(Packet* packet) const final;

			virtual void InitializeHeader(PacketType packetType, uint16_t length) final;

			virtual uint8_t GetCount() const final
			{
				return GetCommonHeaderPointer()->count;
			}

			virtual void SetCount(uint8_t count) final
			{
				GetCommonHeaderPointer()->count = count;
			}

			/**
			 * Value of the Length field, which is the length of the RTCP packet in
			 * 32-bit words minus one, including the Common Header and any padding.
			 * Therefore the total size in bytes is (Length + 1) * 4.
			 */
			virtual uint16_t GetLengthField() const final
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 2);
			}

			/**
			 * Computed value (in bytes) of the Length field.
			 */
			virtual size_t GetLengthFieldComputed() const final
			{
				return (static_cast<size_t>(GetLengthField()) + 1) * 4;
			}

			/**
			 * Set the Length field given the total packet length in bytes. It
			 * encodes it as 32-bit words minus one as required by the RTCP Length
			 * field.
			 *
			 * @throw MediaSoupTypeError - If given `length` is higher than the maximum
			 *   representable packet length (262144 bytes).
			 */
			virtual void SetLengthField(size_t length) final;

			/**
			 * A pointer to the position in the buffer where the variable-length value
			 * (if any) starts or should start.
			 */
			virtual uint8_t* GetVariableLengthValuePointer() const final
			{
				return const_cast<uint8_t*>(GetBuffer()) + Packet::CommonHeaderLength;
			}

			/**
			 * Whether this packet contains a variable-length value.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual bool HasVariableLengthValue() const final
			{
				return GetLengthFieldComputed() > Packet::CommonHeaderLength;
			}

			/**
			 * Variable-length value of this packet.
			 */
			virtual const uint8_t* GetVariableLengthValue() const final
			{
				if (!HasVariableLengthValue())
				{
					return nullptr;
				}

				return GetVariableLengthValuePointer();
			}

			/**
			 * Set the variable-length value. It copies the given value into the
			 * the variable-length value of the packet and updates both the length of
			 * the Serializable and the length field.
			 *
			 * @throw MediaSoupTypeError - If given `valueLength` is higher than
			 *   available length.
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual void SetVariableLengthValue(const uint8_t* value, size_t valueLength) final;

			/**
			 * The length of the variable-length value.
			 */
			virtual size_t GetVariableLengthValueLength() const final
			{
				if (!HasVariableLengthValue())
				{
					return 0u;
				}

				return GetLengthFieldComputed() - Packet::CommonHeaderLength;
			}

			/**
			 * Set the length of the variable-length value. It doesn't copy any value
			 * into the variable-length value. This method is used in packets that
			 * have variable-length value but it doesn't consist on a buffer + length,
			 * but instead is an structure with fields (with variable length).
			 *
			 * @see GetVariableLengthValue()
			 */
			virtual void SetVariableLengthValueLength(size_t valueLength) final;

		private:
			/**
			 * @remarks
			 * - Returns CommonHeader* instead of const CommonHeader* since we may want
			 *   to modify its fields.
			 */
			CommonHeader* GetCommonHeaderPointer() const
			{
				return reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
