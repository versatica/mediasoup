#ifndef MS_RTC_SCTP_HEARTBEAT_CHUNK_HPP
#define MS_RTC_SCTP_HEARTBEAT_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Heartbeat Request Chunk (HEARTBEAT) (4).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 4    |  Chunk Flags  |       Heartbeat Length        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /          Heartbeat Information TLV (Variable-Length)          /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 4.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits).
		 * - Heartbeat Information (variable length).
		 *
		 * Mandatory Variable-Length Parameters:
		 * - Heartbeat Info (1), mandatory.
		 */

		// Forward declaration.
		class Packet;

		class HeartbeatChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a HeartbeatChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static HeartbeatChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a HeartbeatChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static HeartbeatChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a HeartbeatChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static HeartbeatChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			HeartbeatChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~HeartbeatChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual HeartbeatChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool CanHaveParameters() const final
			{
				return true;
			}

		protected:
			virtual HeartbeatChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
