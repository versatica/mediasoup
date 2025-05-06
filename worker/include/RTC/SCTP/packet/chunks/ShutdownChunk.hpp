#ifndef MS_RTC_SCTP_SHUTDOWN_CHUNK_HPP
#define MS_RTC_SCTP_SHUTDOWN_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Shutdown Association Chunk (SHUTDOWN) (7).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 7    |  Chunk Flags  |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      Cumulative TSN Ack                       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 7.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits): 8.
		 * - Cumulative TSN Ack (32 bits): The largest TSN, such that all TSNs
		 *   smaller than or equal to it have been received and the next one has
		 *   not been received.
		 */

		// Forward declaration.
		class Packet;

		class ShutdownChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t ShutdownChunkHeaderLength{ 8 };

		public:
			/**
			 * Parse a ShutdownChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ShutdownChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ShutdownChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static ShutdownChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ShutdownChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ShutdownChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ShutdownChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ShutdownChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ShutdownChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetCumulativeTsnAck() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetCumulativeTsnAck(uint32_t value);

		protected:
			virtual ShutdownChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk doesn't
			 * have variable-length value (despite the fixed header doesn't have
			 * default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return ShutdownChunk::ShutdownChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
