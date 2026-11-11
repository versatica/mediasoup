#ifndef MS_RTC_RTCP_REMB_PACKET_HPP
#define MS_RTC_RTCP_REMB_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "Utils.hpp"
#include <vector>

namespace RTC
{
	namespace RTCP
	{
		/**
		 * Receiver Estimated Maximum Bitrate Application Layer Feedback Packet
		 * (PSFB 206, FMT 15).
		 *
		 * @see draft-alvestrand-rmcat-remb-03.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P| FMT=15  |  PT=PSFB=206  |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                    SSRC of media source = 0                   |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |              Unique identifier 'R' 'E' 'M' 'B'                |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Num SSRC    |  BR Exp   |           BR Mantissa             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         SSRC feedback                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * :                              ...                              :
		 *
		 * - Unique identifier (32 bits): Always 'R' 'E' 'M' 'B'.
		 * - Num SSRC (8 bits): Number of SSRC feedback entries.
		 * - BR Exp (6 bits): Exponential scaling of the mantissa for the maximum
		 *   total media bitrate.
		 * - BR Mantissa (18 bits): Unsigned mantissa of the maximum total media
		 *   bitrate (in bps) of all media streams in the session.
		 * - SSRC feedback (32 bits each): SSRCs of the media streams this
		 *   estimation applies to.
		 */
		class RembPacket : public AnyPayloadSpecificFeedbackPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Value of the Unique identifier field ('R' 'E' 'M' 'B').
			 */
			static constexpr uint32_t UniqueIdentifier{ 0x52454D42 };
			/**
			 * Length (in bytes) of the fixed header that precedes the SSRCs, RTCP
			 * Common Header included.
			 */
			static constexpr size_t RembHeaderLength{ 20 };
			/**
			 * Maximum number of SSRCs in a packet (the Num SSRC field has 8 bits).
			 */
			static constexpr size_t MaxSsrcs{ 255 };

		public:
			/**
			 * Whether the given buffer holds a REMB packet, this is, a Payload-Specific
			 * Feedback packet of Application Layer Feedback type whose Unique
			 * identifier is 'R' 'E' 'M' 'B'.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 * - The rest of the packet is not validated, Parse() does it.
			 */
			static bool IsRemb(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a RembPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static RembPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a RembPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `RembHeaderLength`.
			 */
			static RembPacket* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a RembPacket with the given number of SSRCs.
			 *
			 * @remarks
			 * - Defined here so that it can be used where a constant is needed, such
			 *   as the size of a buffer.
			 */
			static constexpr size_t ComputeLengthFor(size_t numSsrcs)
			{
				// Each SSRC takes 4 bytes, so the packet never needs padding.
				return RembPacket::RembHeaderLength + (numSsrcs * 4);
			}

		private:
			/**
			 * Parse a RembPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static RembPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			RembPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~RembPacket() override;

			void Dump(int indentation = 0) const final;

			RembPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * Maximum total media bitrate (in bps).
			 */
			int64_t GetBitrate() const;

			/**
			 * @throw MediaSoupTypeError - If given `bitrate` is negative.
			 */
			void SetBitrate(int64_t bitrate);

			/**
			 * Value of the Num SSRC field.
			 */
			uint8_t GetNumberOfSsrcs() const
			{
				return Utils::Byte::Get1Byte(GetBuffer(), 16);
			}

			std::vector<uint32_t> GetSsrcs() const;

			/**
			 * Add a SSRC at the end of the packet.
			 *
			 * @throw MediaSoupTypeError - If the packet already has `MaxSsrcs` SSRCs
			 *   or if there is no space in the buffer.
			 */
			void AddSsrc(uint32_t ssrc);

		private:
			RembPacket* SoftClone(const uint8_t* buffer) const final;

			uint8_t* GetSsrcsPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + RembPacket::RembHeaderLength;
			}
		};
	} // namespace RTCP
} // namespace RTC

#endif
