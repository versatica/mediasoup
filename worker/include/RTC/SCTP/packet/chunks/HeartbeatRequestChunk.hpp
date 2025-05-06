#ifndef MS_RTC_SCTP_HEARTBEAT_REQUEST_CHUNK_HPP
#define MS_RTC_SCTP_HEARTBEAT_REQUEST_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Heartbeat Request Chunk (HEARTBEAT_REQUEST) (4).
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

		class HeartbeatRequestChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a HeartbeatRequestChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static HeartbeatRequestChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a HeartbeatRequestChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static HeartbeatRequestChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a HeartbeatRequestChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static HeartbeatRequestChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			HeartbeatRequestChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~HeartbeatRequestChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual HeartbeatRequestChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool CanHaveParameters() const final
			{
				return true;
			}

		protected:
			virtual HeartbeatRequestChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
