#ifndef MS_RTC_NEW_RTCP_RRT_EXTENDED_REPORT_HPP
#define MS_RTC_NEW_RTCP_RRT_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP XR Receiver Reference Time Report Block (4).
		 *
		 * @see RFC 3611.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     BT=4      |   reserved    |       block length = 2        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |              NTP timestamp, most significant word             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |             NTP timestamp, least significant word             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Block type (BT) (8 bits): 4.
		 * - Reserved (8 bits): Must be set to zero and must be ignored by the
		 *   receiver.
		 * - Block length (16 bits): The constant 2.
		 * - NTP timestamp (64 bits): Wallclock time when this block was sent.
		 */

		class RrtExtendedReport : public ExtendedReport
		{
			// We need that XrPacket calls protected and private methods in this
			// class.
			friend class XrPacket;

		public:
			/**
			 * Length (in bytes) of the Receiver Reference Time Report Block.
			 */
			static constexpr size_t RrtExtendedReportLength{ 12 };

		public:
			/**
			 * Parse a RrtExtendedReport.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Extended Report
			 *   block.
			 */
			static RrtExtendedReport* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a RrtExtendedReport.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Extended Report block real
			 *   length.
			 *
			 * @throw MediaSoupTypeError - If given `bufferLength` is lower than the
			 *   Receiver Reference Time Report Block length (12 bytes).
			 */
			static RrtExtendedReport* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a RrtExtendedReport.
			 *
			 * @remarks
			 * - To be used only by `XrPacket::ParseStrict()`.
			 */
			static RrtExtendedReport* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength);

		private:
			/**
			 * Only used by ParseStrict(), Factory(), Clone() and SoftClone() methods.
			 */
			RrtExtendedReport(uint8_t* buffer, size_t bufferLength);

		public:
			~RrtExtendedReport() override;

			void Dump(int indentation = 0) const final;

			RrtExtendedReport* Clone(uint8_t* buffer, size_t bufferLength) const final;

			/**
			 * NTP timestamp.
			 */
			Utils::Time::Ntp GetNtp() const
			{
				return { .seconds   = Utils::Byte::Get4Bytes(GetBuffer(), 4),
				         .fractions = Utils::Byte::Get4Bytes(GetBuffer(), 8) };
			}

			void SetNtp(const Utils::Time::Ntp& ntp);

		private:
			RrtExtendedReport* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
