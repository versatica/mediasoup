#ifndef MS_RTC_SCTP_UNKNOWN_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_UNKNOWN_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unknown Error Cause (UNKNOWN)
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Cause Code           |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                         Unknown Value                         /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnknownErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnknownErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static UnknownErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnknownErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static UnknownErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnknownErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnknownErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnknownErrorCause* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnknownCode() const override
			{
				return true;
			}

			virtual bool HasUnknownValue() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnknownValueLength() const
			{
				return GetVariableLengthValueLength();
			}

		protected:
			virtual UnknownErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
