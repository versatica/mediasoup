#ifndef MS_RTC_SCTP_HEARTBEAT_INFO_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_HEARTBEAT_INFO_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * HeartberatInfo Chunk Parameter (HEARBEAT INFO) (1).
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Heartbeat Info Type = 1    |        HB Info Length         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                Sender-Specific Heartbeat Info                 /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class HeartbeatInfoChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a HeartbeatInfoChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static HeartbeatInfoChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a HeartbeatInfoChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Parameter real length.
			 */
			static HeartbeatInfoChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			HeartbeatInfoChunkParameter(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~HeartbeatInfoChunkParameter() override;

			virtual void Dump() const override final;

			virtual HeartbeatInfoChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			const uint8_t* GetInfo() const
			{
				if (!HasValue())
				{
					return nullptr;
				}

				return GetValuePointer();
			}

			void SetInfo(const uint8_t* info, size_t infoLength);
		};
	} // namespace SCTP
} // namespace RTC

#endif
