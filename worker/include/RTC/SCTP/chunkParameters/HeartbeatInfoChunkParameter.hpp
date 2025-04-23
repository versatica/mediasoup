#ifndef MS_RTC_SCTP_HEARTBEAT_INFO_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_HEARTBEAT_INFO_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP HeartberatInfo Chunk Parameter (HEARBEAT INFO) (1).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 1            |        HB Info Length         |
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
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static HeartbeatInfoChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a HeartbeatInfoChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static HeartbeatInfoChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a HeartbeatInfoChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static HeartbeatInfoChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			HeartbeatInfoChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~HeartbeatInfoChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual HeartbeatInfoChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasInfo() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetInfo() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetInfoLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetInfo(const uint8_t* info, uint16_t infoLength);

		protected:
			virtual HeartbeatInfoChunkParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
