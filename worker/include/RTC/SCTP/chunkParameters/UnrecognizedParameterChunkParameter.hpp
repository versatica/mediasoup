#ifndef MS_RTC_SCTP_UNRECOGNIZED_PARAMETER_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_UNRECOGNIZED_PARAMETER_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP State Cookie Chunk Parameter (UNRECOGNIZED_PARAMETER) (7).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 8            |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                    Unrecognized Parameter                     /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnrecognizedParameterChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnrecognizedParameterChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static UnrecognizedParameterChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UnrecognizedParameterChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static UnrecognizedParameterChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnrecognizedParameterChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static UnrecognizedParameterChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			UnrecognizedParameterChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnrecognizedParameterChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnrecognizedParameterChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnrecognizedParameter() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnrecognizedParameter() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnrecognizedParameterLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUnrecognizedParameter(const uint8_t* parameter, uint16_t parameterLength);

		protected:
			virtual UnrecognizedParameterChunkParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
