#ifndef MS_RTC_NEW_RTCP_SENDER_REPORT_PACKET_HPP
#define MS_RTC_NEW_RTCP_SENDER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyReportPacket.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Sender Report RTCP Packet (200).
		 *
		 * @see RFC 3550.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|    RC   |   PT=SR=200   |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         SSRC of sender                        |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |              NTP timestamp, most significant word             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |             NTP timestamp, least significant word             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         RTP timestamp                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     sender's packet count                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      sender's octet count                     |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                        Report block 1                         |
		 * |                              ...                              |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                        Report block 2                         |
		 * |                              ...                              |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                  profile-specific extensions                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Packet type (8 bits): 200.
		 * - Reception report count (RC) (5 bits): The number of report blocks
		 *   contained in this packet.
		 * - SSRC of sender (32 bits).
		 * - NTP timestamp (64 bits): Wallclock time when this report was sent.
		 * - RTP timestamp (32 bits): Same time as the NTP timestamp but in the
		 *   same units and with the same random offset as the RTP timestamps in
		 *   data packets.
		 * - Sender's packet count (32 bits): Total number of RTP data packets
		 *   transmitted by the sender since starting transmission.
		 * - Sender's octet count (32 bits): Total number of payload octets
		 *   transmitted by the sender since starting transmission.
		 * - Report blocks: See AnyReportPacket.
		 */
		class SenderReportPacket : public AnyReportPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Length (in bytes) of the fixed header that precedes the report blocks,
			 * Common Header included.
			 */
			static constexpr size_t SenderReportHeaderLength{ 28 };

		public:
			/**
			 * Parse a SenderReportPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static SenderReportPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SenderReportPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `SenderReportHeaderLength`.
			 */
			static SenderReportPacket* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SenderReportPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static SenderReportPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			SenderReportPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~SenderReportPacket() override;

			void Dump(int indentation = 0) const final;

			SenderReportPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * Most significant word of the NTP timestamp.
			 */
			uint32_t GetNtpSec() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			void SetNtpSec(uint32_t ntpSec);

			/**
			 * Least significant word of the NTP timestamp.
			 */
			uint32_t GetNtpFrac() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 12);
			}

			void SetNtpFrac(uint32_t ntpFrac);

			uint32_t GetRtpTimestamp() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 16);
			}

			void SetRtpTimestamp(uint32_t rtpTimestamp);

			/**
			 * Sender's packet count.
			 */
			uint32_t GetPacketCount() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 20);
			}

			void SetPacketCount(uint32_t packetCount);

			/**
			 * Sender's octet count.
			 */
			uint32_t GetOctetCount() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 24);
			}

			void SetOctetCount(uint32_t octetCount);

		private:
			SenderReportPacket* SoftClone(const uint8_t* buffer) const final;

			size_t GetHeaderLength() const final
			{
				return SenderReportPacket::SenderReportHeaderLength;
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
