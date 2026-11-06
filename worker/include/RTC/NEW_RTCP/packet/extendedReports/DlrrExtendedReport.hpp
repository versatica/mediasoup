#ifndef MS_RTC_NEW_RTCP_DLRR_EXTENDED_REPORT_HPP
#define MS_RTC_NEW_RTCP_DLRR_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP XR DLRR Report Block (5).
		 *
		 * @see RFC 3611.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     BT=5      |   reserved    |         block length          |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                 SSRC_1 (SSRC of first receiver)               | sub-
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
		 * |                         last RR (LRR)                         |   1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                   delay since last RR (DLRR)                  |
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 * |                 SSRC_2 (SSRC of second receiver)              | sub-
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
		 * :                               ...                             :   2
		 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
		 *
		 * - Block type (BT) (8 bits): 5.
		 * - Reserved (8 bits): Must be set to zero and must be ignored by the
		 *   receiver.
		 * - Sub-blocks: One per Receiver Report. See SubBlock.
		 */

		class DlrrExtendedReport : public ExtendedReport
		{
			// We need that XrPacket calls protected and private methods in this
			// class.
			friend class XrPacket;

		public:
			/**
			 * Length (in bytes) of a sub-block.
			 */
			static constexpr size_t SubBlockLength{ 12 };

		public:
			/**
			 * DLRR sub-block.
			 */
			struct SubBlock
			{
				/**
				 * SSRC of the receiver.
				 */
				uint32_t ssrc;
				/**
				 * Last RR timestamp (LRR): The middle 32 bits of the NTP timestamp
				 * received in the last Receiver Reference Time Report Block from the
				 * receiver (0 if none was received).
				 */
				uint32_t lastReceiverReport;
				/**
				 * Delay since last RR (DLRR): Delay, in units of 1/65536 seconds,
				 * between receiving the last Receiver Reference Time Report Block from
				 * the receiver and sending this DLRR Report Block (0 if none was
				 * received).
				 */
				uint32_t delaySinceLastReceiverReport;
			};

		public:
			/**
			 * Parse a DlrrExtendedReport.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Extended Report
			 *   block.
			 */
			static DlrrExtendedReport* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a DlrrExtendedReport.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Extended Report block real
			 *   length.
			 * - The DlrrExtendedReport is created without sub-blocks.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than the
			 *   Extended Report block header length (4 bytes).
			 */
			static DlrrExtendedReport* Factory(uint8_t* buffer, size_t bufferLength);

			/**
			 * Length (in bytes) of a DlrrExtendedReport with the given number of
			 * sub-blocks.
			 */
			static size_t ComputeLengthFor(size_t numSubBlocks);

		private:
			/**
			 * Parse a DlrrExtendedReport.
			 *
			 * @remarks
			 * - To be used only by `XrPacket::ParseStrict()`.
			 */
			static DlrrExtendedReport* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			DlrrExtendedReport(uint8_t* buffer, size_t bufferLength);

		public:
			~DlrrExtendedReport() override;

			void Dump(int indentation = 0) const final;

			DlrrExtendedReport* Clone(uint8_t* buffer, size_t bufferLength) const final;

			size_t GetNumberOfSubBlocks() const
			{
				return GetVariableLengthValueLength() / DlrrExtendedReport::SubBlockLength;
			}

			std::vector<SubBlock> GetSubBlocks() const;

			/**
			 * Add a sub-block at the end of the Extended Report block.
			 *
			 * @throw MediaSoupTypeError - If there is no space in the buffer.
			 */
			void AddSubBlock(const SubBlock& subBlock);

		private:
			DlrrExtendedReport* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
