#ifndef MS_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_PARAMETER_HPP
#define MS_RTC_SCTP_ZERO_CHECKSUM_ACCEPTABLE_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Zero Checksum Acceptable Parameter (ZERO_CHECKSUM_ACCEPTABLE)
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

		class ZeroChecksumAcceptableParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Zero Checksum Alternate Error Detection Method.
			 */
			enum class AlternateErrorDetectionMethod : uint32_t
			{
				NONE           = 0x0000,
				SCTP_OVER_DTLS = 0x0001
			};

		public:
			static const size_t ZeroChecksumAcceptableParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a ZeroChecksumAcceptableParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static ZeroChecksumAcceptableParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ZeroChecksumAcceptableParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static ZeroChecksumAcceptableParameter* Factory(uint8_t* buffer, size_t bufferLength);

			static const std::string& AlternateErrorDetectionMethod2String(
			  AlternateErrorDetectionMethod alternateErrorDetectionMethod);

		private:
			/**
			 * Parse a ZeroChecksumAcceptableParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static ZeroChecksumAcceptableParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

			static std::unordered_map<AlternateErrorDetectionMethod, std::string>
			  alternateErrorDetectionMethod2String;

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ZeroChecksumAcceptableParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ZeroChecksumAcceptableParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ZeroChecksumAcceptableParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			AlternateErrorDetectionMethod GetAlternateErrorDetectionMethod() const
			{
				return static_cast<AlternateErrorDetectionMethod>(Utils::Byte::Get4Bytes(GetBuffer(), 4));
			}

			void SetAlternateErrorDetectionMethod(AlternateErrorDetectionMethod alternateErrorDetectionMethod);

		protected:
			virtual ZeroChecksumAcceptableParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
