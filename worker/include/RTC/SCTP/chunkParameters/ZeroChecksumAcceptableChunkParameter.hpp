#ifndef MS_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Zero Checksum Acceptable Chunk Parameter (ZERO_CHECKSUM_ACCEPTABLE)
		 * (32769).
		 *
		 * @see RFC 9653.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Type = 0x8001        |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Error Detection Method Identifier (EDMID)           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class ZeroChecksumAcceptableChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t ZeroChecksumAcceptableChunkParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a ZeroChecksumAcceptableChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static ZeroChecksumAcceptableChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ZeroChecksumAcceptableChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static ZeroChecksumAcceptableChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ZeroChecksumAcceptableChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static ZeroChecksumAcceptableChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ZeroChecksumAcceptableChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ZeroChecksumAcceptableChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ZeroChecksumAcceptableChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetEdmid() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetEdmid(uint32_t value);

		protected:
			virtual ZeroChecksumAcceptableChunkParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return ZeroChecksumAcceptableChunkParameter::ZeroChecksumAcceptableChunkParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
