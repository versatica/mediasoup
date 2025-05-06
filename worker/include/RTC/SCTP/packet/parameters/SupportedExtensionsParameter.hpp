#ifndef MS_RTC_SCTP_SUPPORTED_EXTENSIONS_PARAMETER_HPP
#define MS_RTC_SCTP_SUPPORTED_EXTENSIONS_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Supported Extensions Parameter (SUPPORTED_EXTENSIONS) (32776).
		 *
		 * @see RFC 5061.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Parameter Type = 0x8008   |      Parameter Length         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | CHUNK TYPE 1  |  CHUNK TYPE 2 |  CHUNK TYPE 3 |  CHUNK TYPE 4 |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                             ....                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | CHUNK TYPE N  |      PAD      |      PAD      |      PAD      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class SupportedExtensionsParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a SupportedExtensionsParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static SupportedExtensionsParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SupportedExtensionsParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static SupportedExtensionsParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SupportedExtensionsParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static SupportedExtensionsParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			SupportedExtensionsParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~SupportedExtensionsParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual SupportedExtensionsParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint16_t GetNumberOfChunkTypes() const
			{
				return GetVariableLengthValueLength();
			}

			Chunk::ChunkType GetChunkTypeAt(uint16_t idx) const
			{
				return static_cast<Chunk::ChunkType>(
				  Utils::Byte::Get1Byte(GetVariableLengthValuePointer(), idx));
			}

			void AddChunkType(Chunk::ChunkType chunkType);

		protected:
			virtual SupportedExtensionsParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
