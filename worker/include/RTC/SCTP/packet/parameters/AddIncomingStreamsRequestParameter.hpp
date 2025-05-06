#ifndef MS_RTC_SCTP_ADD_INCOMING_STREAMS_REQUEST_PARAMETER_HPP
#define MS_RTC_SCTP_ADD_INCOMING_STREAMS_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Add Incoming Streams Request Parameter
		 * (ADD_INCOMING_STREAMS_REQUEST) (18).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Parameter Type = 18       |      Parameter Length = 12    |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Re-configuration Request Sequence Number             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      Number of new streams    |        (Reserved)             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class AddIncomingStreamsRequestParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t AddIncomingStreamsRequestParameterHeaderLength{ 12 };

		public:
			/**
			 * Parse a AddIncomingStreamsRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static AddIncomingStreamsRequestParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a AddIncomingStreamsRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static AddIncomingStreamsRequestParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a AddIncomingStreamsRequestParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static AddIncomingStreamsRequestParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			AddIncomingStreamsRequestParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~AddIncomingStreamsRequestParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual AddIncomingStreamsRequestParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetReconfigurationRequestSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationRequestSequenceNumber(uint32_t value);

			uint16_t GetNumberOfNewStreams() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 8);
			}

			void SetNumberOfNewStreams(uint16_t value);

		protected:
			virtual AddIncomingStreamsRequestParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return AddIncomingStreamsRequestParameter::AddIncomingStreamsRequestParameterHeaderLength;
			}

		private:
			void SetReserved();
		};
	} // namespace SCTP
} // namespace RTC

#endif
