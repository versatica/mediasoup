#ifndef MS_RTC_SCTP_UNRECOGNIZED_CHUNK_TYPE_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_UNRECOGNIZED_CHUNK_TYPE_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unrecognized Chunk Type Error Cause (UNRECOGNIZED_CHUNK_TYPE) (6)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 6         |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                      Unrecognized Chunk                       /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnrecognizedChunkTypeErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnrecognizedChunkTypeErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static UnrecognizedChunkTypeErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UnrecognizedChunkTypeErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static UnrecognizedChunkTypeErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnrecognizedChunkTypeErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static UnrecognizedChunkTypeErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnrecognizedChunkTypeErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnrecognizedChunkTypeErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnrecognizedChunkTypeErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnrecognizedChunk() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnrecognizedChunk() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnrecognizedChunkLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUnrecognizedChunk(const uint8_t* chunk, uint16_t chunkLength);

		protected:
			virtual UnrecognizedChunkTypeErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
