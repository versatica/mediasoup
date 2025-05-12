#ifndef MS_RTC_SCTP_SHUTDOWN_COMPLETE_CHUNK_HPP
#define MS_RTC_SCTP_SHUTDOWN_COMPLETE_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Shutdown Complete Chunk (SHUTDOWN_COMPLETE) (14).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 14   |  Reserved   |T|          Length = 4           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 14
		 * - T bit (1 bit): The T bit is set to 0 if the sender filled in the
		 *   Verification Tag expected by the peer. If the Verification Tag is
		 *   reflected, the T bit MUST be set to 1. Reflecting means that the sent
		 *   Verification Tag is the same as the received one.
		 * - Length (16 bits): 4.
		 */

		// Forward declaration.
		class Packet;

		class ShutdownCompleteChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a ShutdownCompleteChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ShutdownCompleteChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ShutdownCompleteChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static ShutdownCompleteChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ShutdownCompleteChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ShutdownCompleteChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ShutdownCompleteChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ShutdownCompleteChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ShutdownCompleteChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			bool GetT() const
			{
				return GetBit0();
			}

			void SetT(bool flag);

		protected:
			virtual ShutdownCompleteChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
