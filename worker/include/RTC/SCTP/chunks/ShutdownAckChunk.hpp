#ifndef MS_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP
#define MS_RTC_SCTP_SHUTDOWN_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Shutdown Acknowledgement (SHUTDOWN ACK) (8).
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 8    |  Chunk Flags  |          Length = 4           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 8.
		 * - Length (16 bits): 4.
		 */

		// Forward declaration.
		class Packet;

		class ShutdownAckChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t ShutdownAckChunkLength{ 4 };

		public:
			/**
			 * Parse a ShutdownAckChunk.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ShutdownAckChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ShutdownAckChunk.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Chunk real length.
			 */
			static ShutdownAckChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			ShutdownAckChunk(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ShutdownAckChunk() override;

			virtual void Dump() const override final;

			virtual ShutdownAckChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
