#ifndef MS_RTC_SCTP_ABORT_ASSOCIATION_ERROR_CHUNK_HPP
#define MS_RTC_SCTP_ABORT_ASSOCIATION_ERROR_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Abort Association Chunk (ABORT) (6).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 6    |  Reserved   |T|            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                   zero or more Error Causes                   /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 6.
		 * - T bit (1 bit): The T bit is set to 0 if the sender filled in the
		 *   Verification Tag expected by the peer. If the Verification Tag is
		 *   reflected, the T bit MUST be set to 1. Reflecting means that the sent
		 *   Verification Tag is the same as the received one.
		 * - Length (16 bits).
		 *
		 * Optional Variable-Length Error Causes (anyone).
		 */

		// Forward declaration.
		class Packet;

		class AbortAssociationChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a AbortAssociationChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static AbortAssociationChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a AbortAssociationChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static AbortAssociationChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a AbortAssociationChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static AbortAssociationChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			AbortAssociationChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~AbortAssociationChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual AbortAssociationChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			bool GetT() const
			{
				return GetBit0();
			}

			void SetT(bool flag);

			virtual bool CanHaveErrorCauses() const final
			{
				return true;
			}

		protected:
			virtual AbortAssociationChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
