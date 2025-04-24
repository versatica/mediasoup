#ifndef MS_RTC_SCTP_INCOMING_SSN_RESET_REQUEST_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_INCOMING_SSN_RESET_REQUEST_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Incoming SSN Reset Request Chunk Parameter
		 * (INCOMING_SSN_RESET_REQUEST) (14).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Parameter Type = 14       |  Parameter Length = 8 + 2 * N |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Re-configuration Request Sequence Number             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Stream Number 1 (optional)   |    Stream Number 2 (optional) |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                            ......                             /
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Stream Number N-1 (optional) |    Stream Number N (optional) |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class IncomingSsnResetRequestChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t IncomingSsnResetRequestChunkParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a IncomingSsnResetRequestChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static IncomingSsnResetRequestChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IncomingSsnResetRequestChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static IncomingSsnResetRequestChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IncomingSsnResetRequestChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static IncomingSsnResetRequestChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IncomingSsnResetRequestChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IncomingSsnResetRequestChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IncomingSsnResetRequestChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetReconfigurationRequestSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationRequestSequenceNumber(uint32_t value);

			uint16_t GetNumberOfStreams() const
			{
				return GetVariableLengthValueLength() / 2;
			}

			uint16_t GetStreamAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 2));
			}

			void AddStream(uint16_t stream);

		protected:
			virtual IncomingSsnResetRequestChunkParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IncomingSsnResetRequestChunkParameter::IncomingSsnResetRequestChunkParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
