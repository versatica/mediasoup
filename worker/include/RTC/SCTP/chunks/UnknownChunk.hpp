#ifndef MS_RTC_SCTP_DATA_UNKNOWN_HPP
#define MS_RTC_SCTP_DATA_UNKNOWN_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Unknown Chunk.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Chunk Type   |  Chunk Flags  |         Chunk Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Unknown Value                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Packet;

		class UnknownChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a UnknownChunk.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static UnknownChunk* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() static method.
			 */
			UnknownChunk(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnknownChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnknownChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnknownType() const override
			{
				return true;
			}

			bool HasUnknownValue() const
			{
				return GetLengthField() > Chunk::ChunkHeaderLength;
			}

			const uint8_t* GetUnknownValue() const
			{
				if (!HasUnknownValue())
				{
					return nullptr;
				}

				return GetValuePointer();
			}

			size_t GetUnknownValueLength() const
			{
				if (!HasUnknownValue())
				{
					return 0u;
				}

				return GetLengthField() - Chunk::ChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
