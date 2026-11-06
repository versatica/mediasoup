#ifndef MS_RTC_NEW_RTCP_RECEIVER_REPORT_PACKET_HPP
#define MS_RTC_NEW_RTCP_RECEIVER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/AnyReportPacket.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * Receiver Report RTCP Packet (201).
		 *
		 * @see RFC 3550.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |V=2|P|    RC   |   PT=RR=201   |             Length            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                     SSRC of packet sender                     |
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
		 * - Packet type (8 bits): 201.
		 * - Reception report count (RC) (5 bits): The number of report blocks
		 *   contained in this packet.
		 * - SSRC of packet sender (32 bits).
		 * - Report blocks: See AnyReportPacket.
		 */
		class ReceiverReportPacket : public AnyReportPacket
		{
			// We need that compound packet calls protected and private methods in
			// this class.
			friend class CompoundPacket;

		public:
			/**
			 * Length (in bytes) of the fixed header that precedes the report blocks,
			 * Common Header included.
			 */
			static constexpr size_t ReceiverReportHeaderLength{ 8 };

		public:
			/**
			 * Parse a ReceiverReportPacket.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the packet.
			 */
			static ReceiverReportPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ReceiverReportPacket.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the packet real length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than
			 *   `ReceiverReportHeaderLength`.
			 */
			static ReceiverReportPacket* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a ReceiverReportPacket with the given number of
			 * report blocks and no profile-specific extensions.
			 */
			static size_t ComputeLengthFor(size_t numReportBlocks);

		private:
			/**
			 * Parse a ReceiverReportPacket.
			 *
			 * @remarks
			 * - To be used only by `CompoundPacket::Parse()`.
			 */
			static ReceiverReportPacket* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			ReceiverReportPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~ReceiverReportPacket() override;

			void Dump(int indentation = 0) const final;

			ReceiverReportPacket* Clone(uint8_t* buffer, size_t bufferLength) const final;

		private:
			ReceiverReportPacket* SoftClone(const uint8_t* buffer) const final;

			size_t GetHeaderLength() const final
			{
				return ReceiverReportPacket::ReceiverReportHeaderLength;
			}
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
