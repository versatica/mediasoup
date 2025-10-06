#ifndef MS_RTC_SCTP_OPERATION_ERROR_CHUNK_HPP
#define MS_RTC_SCTP_OPERATION_ERROR_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Operation Error Chunk (OPERATION_ERROR) (9).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 9    |  Chunk Flags  |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                   one or more Error Causes                    /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 9.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits).
		 *
		 * Optional Variable-Length Error Causes (anyone).
		 */

		// Forward declaration.
		class Packet;

		class OperationErrorChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a OperationErrorChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static OperationErrorChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a OperationErrorChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static OperationErrorChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a OperationErrorChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static OperationErrorChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			OperationErrorChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~OperationErrorChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual OperationErrorChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool CanHaveErrorCauses() const final
			{
				return true;
			}

		protected:
			virtual OperationErrorChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
