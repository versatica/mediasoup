#ifndef MS_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP
#define MS_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Shutdown Acknowledgement Chunk (SHUTDOWN_ACK) (8).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 8    |  Chunk Flags  |          Length = 4           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 8.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits): 4.
		 */

		// Forward declaration.
		class Packet;

		class ShutdownAckChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a ShutdownAckChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ShutdownAckChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ShutdownAckChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static ShutdownAckChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ShutdownAckChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ShutdownAckChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ShutdownAckChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ShutdownAckChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ShutdownAckChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

		protected:
			virtual ShutdownAckChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
