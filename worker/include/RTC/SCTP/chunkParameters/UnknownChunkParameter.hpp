#ifndef MS_RTC_SCTP_UNKNOWN_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_UNKNOWN_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Unknown Chunk Parameter.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Parameter Type         |       Parameter Length        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                        Parameter Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnknownChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnknownChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static UnknownChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			UnknownChunkParameter(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnknownChunkParameter() override;

			virtual void Dump() const override final;

			virtual UnknownChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnknownType() const override
			{
				return true;
			}

			const uint8_t* GetUnknownValue() const
			{
				if (!HasValue())
				{
					return nullptr;
				}

				return GetValuePointer();
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
