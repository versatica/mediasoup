#ifndef MS_RTC_SCTP_RE_CONFIG_CHUNK_HPP
#define MS_RTC_SCTP_RE_CONFIG_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Re-Config Chunk (RE_CONFIG) (130).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | Type = 130    |  Chunk Flags  |      Chunk Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                  Re-configuration Parameter                   /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /             Re-configuration Parameter (optional)             /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 130.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits).
		 * - Re-configuration Parameter (variable Length): This field holds a
		 *   Re-configuration Request Parameter or a Re-configuration Response
		 *   Parameter.
		 *
		 * Variable-Length Parameters:
		 * - Re-configuration Request Parameter or a Re-configuration Response
		 *   Parameter, mandatory.
		 * - Another Parameter, optional.
		 */

		// Forward declaration.
		class Packet;

		class ReConfigChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a ReConfigChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ReConfigChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ReConfigChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static ReConfigChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ReConfigChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ReConfigChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ReConfigChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ReConfigChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ReConfigChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool CanHaveParameters() const final
			{
				return true;
			}

		protected:
			virtual ReConfigChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
