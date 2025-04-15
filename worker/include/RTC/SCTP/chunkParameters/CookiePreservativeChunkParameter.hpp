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
		 * Cookie Preservative Chunk Parameter (9).
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
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static CookiePreservativeChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookiePreservativeChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Parameter real length.
			 */
			static CookiePreservativeChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			CookiePreservativeChunkParameter(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookiePreservativeChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookiePreservativeChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetLifeSpanIncrement() const
			{
				return Utils::Byte::Get4Bytes(GetValuePointer(), 0);
			}

			void SetLifeSpanIncrement(const uint32_t increment);
		};
	} // namespace SCTP
} // namespace RTC

#endif
