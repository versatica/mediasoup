#ifndef MS_RTC_NEW_RTCP_UNKNOWN_EXTENDED_REPORT_HPP
#define MS_RTC_NEW_RTCP_UNKNOWN_EXTENDED_REPORT_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP XR Extended Report block with an unknown or not implemented block
		 * type.
		 *
		 * @see RFC 3611.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      BT       | type-specific |         block length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Unknown Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		class UnknownExtendedReport : public ExtendedReport
		{
			// We need that XrPacket calls protected and private methods in this
			// class.
			friend class XrPacket;

		public:
			/**
			 * Parse an UnknownExtendedReport.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Extended Report
			 *   block.
			 */
			static UnknownExtendedReport* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse an UnknownExtendedReport.
			 *
			 * @remarks
			 * - To be used only by `XrPacket::ParseStrict()`.
			 */
			static UnknownExtendedReport* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, size_t extendedReportLength);

		private:
			/**
			 * Only used by ParseStrict(), Clone() and SoftClone() methods.
			 */
			UnknownExtendedReport(uint8_t* buffer, size_t bufferLength);

		public:
			~UnknownExtendedReport() override;

			void Dump(int indentation = 0) const final;

			UnknownExtendedReport* Clone(uint8_t* buffer, size_t bufferLength) const final;

			bool HasUnknownType() const override
			{
				return true;
			}

			bool HasUnknownValue() const
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetVariableLengthValue();
			}

			size_t GetUnknownValueLength() const
			{
				return GetVariableLengthValueLength();
			}

		private:
			UnknownExtendedReport* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
