#ifndef MS_RTC_SCTP_DATA_CHUNK_HPP
#define MS_RTC_SCTP_DATA_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Payload Data Chunk (DATA).
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 0    |  Res  |I|U|B|E|            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              TSN                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      Stream Identifier S      |   Stream Sequence Number n    |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                  Payload Protocol Identifier                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                 User Data (seq n of Stream S)                 /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 0.
		 *
		 * TODO: Complete this.
		 */

		// Forward declaration.
		class Packet;

		class DataChunk : public Chunk
		{
			friend class Packet;

		public:
			static const size_t DataChunkHeaderLength{ 16 };

		public:
			/**
			 * Parse a DataChunk.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static DataChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a DataChunk.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Chunk real length.
			 */
			static DataChunk* Factory(
			  uint8_t* buffer, size_t bufferLength, uint8_t flags, const std::string& text);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			DataChunk(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~DataChunk() override;

			virtual void Dump() const override final;

			virtual DataChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual const uint8_t* GetUserData() const final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
