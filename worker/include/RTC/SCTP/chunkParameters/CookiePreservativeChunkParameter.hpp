#ifndef MS_RTC_SCTP_COOKIE_PRESERVATIVE_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_COOKIE_PRESERVATIVE_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Cookie Preservative Chunk Parameter (COOKIE_PRESERVATIVE) (9).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 9            |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Suggested Cookie Life-Span Increment (msec.)          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class CookiePreservativeChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t CookiePreservativeChunkParameterLength{ 8 };

		public:
			/**
			 * Parse a CookiePreservativeChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static CookiePreservativeChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookiePreservativeChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static CookiePreservativeChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a CookiePreservativeChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static CookiePreservativeChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			CookiePreservativeChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookiePreservativeChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookiePreservativeChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetLifeSpanIncrement() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetLifeSpanIncrement(const uint32_t increment);

		protected:
			virtual CookiePreservativeChunkParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return CookiePreservativeChunkParameter::CookiePreservativeChunkParameterLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
