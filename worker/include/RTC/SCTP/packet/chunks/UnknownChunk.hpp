#ifndef MS_RTC_SCTP_DATA_UNKNOWN_HPP
#define MS_RTC_SCTP_DATA_UNKNOWN_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unknown Chunk (UNKNOWN).
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
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static UnknownChunk* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnknownChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static UnknownChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnknownChunk(uint8_t* buffer, size_t bufferLength);

		public:
			~UnknownChunk() override;

			void Dump(int indentation = 0) const final;

			UnknownChunk* Clone(uint8_t* buffer, size_t bufferLength) const final;

			bool HasUnknownType() const override
			{
				return true;
			}

			bool HasUnknownValue() const
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnknownValueLength() const
			{
				return GetVariableLengthValueLength();
			}

		protected:
			UnknownChunk* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
