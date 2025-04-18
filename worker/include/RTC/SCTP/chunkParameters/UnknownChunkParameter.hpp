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
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static UnknownChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a UnknownChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static UnknownChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			UnknownChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnknownChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnknownChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnknownType() const override
			{
				return true;
			}

			virtual bool HasUnknownValue() const final
			{
				return HasValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetValue();
			}

			uint16_t GetUnknownValueLength() const
			{
				return GetValueLength();
			}

		protected:
			virtual UnknownChunkParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
