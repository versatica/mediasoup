#ifndef MS_RTC_SCTP_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/TLV.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Error Cause.
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Cause Code           |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                  Cause-Specific Information                   /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Cause Code (16 bits).
		 * - Cause Length (16 bits): Set to the size of the Error Cause in bytes,
		 *   including the Cause Code, Cause Length, and Cause-Specific Information
		 *   fields (padding excluded).
		 * - Cause-Specific Information (variable length): This field carries the
		 *   details of the error condition.
		 * - Padding: Bytes of padding to make the Error Cause total length be
		 *   multiple of 4 bytes.
		 */

		// Forward declaration.
		class Chunk;

		class ErrorCause : public TLV
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Error Cause Code.
			 */
			enum class ErrorCauseCode : uint16_t
			{
				INVALID_STREAM_IDENTIFIER                    = 0x0001,
				MISSING_MANDATORY_PARAMETER                  = 0x0002,
				STALE_COOKIE                                 = 0x0003,
				OUT_OF_RESOURCE                              = 0x0004,
				UNRESOLVABLE_ADDRESS                         = 0x0005,
				UNRECOGNIZED_CHUNK_TYPE                      = 0x0006,
				INVALID_MANDATORY_PARAMETER                  = 0x0007,
				UNRECOGNIZED_PARAMETERS                      = 0x0008,
				NO_USER_DATA                                 = 0x0009,
				COOKIE_RECEIVED_WHILE_SHUTTING_DOWN          = 0x000A,
				RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES = 0x000B,
				USER_INITIATED_ABORT                         = 0x000C,
				PROTOCOL_VIOLATION                           = 0x000D,
			};

			/**
			 * Struct of a SCTP Error Cause Header.
			 */
			struct ErrorCauseHeader
			{
				ErrorCauseCode code;
				/**
				 * The value of the Error Cause Length field, which represents the
				 * total length of the Error Cause in bytes, including the Cause Code,
				 * Cause Length and Cause-Specific Information fields. So if the
				 * Cause-Specific Information field is zero-length, the Length field
				 * must be 4. The Cause Length field does not count any padding.
				 */
				uint16_t length;
			};

		public:
			static const size_t ErrorCauseHeaderLength{ 4 };

		public:
			/**
			 * Whether given buffer could be a a valid Error Cause.
			 *
			 * @param buffer
			 * @param bufferLength - Can be greater than real Error Cause length.
			 * @param causeCode - If given buffer is a valid Error Cause then
			 *   `causeCode` is rewritten to parsed ErrorCauseCode.
			 * @param causeLength - If given buffer is a valid Error Cause then
			 *   `causeLength` is rewritten to the value of the Cause Length field.
			 * @param padding - If given buffer is a valid Error Cause then `padding`
			 *   is rewritten to the number of padding bytes in the Error Cause (only
			 *   the necessary ones to make total length multiple of 4).
			 */
			static bool IsErrorCause(
			  const uint8_t* buffer,
			  size_t bufferLength,
			  ErrorCauseCode& causeCode,
			  uint16_t& causeLength,
			  uint8_t& padding);

			static const std::string& ErrorCauseCode2String(ErrorCauseCode causeCode);

		private:
			static std::unordered_map<ErrorCauseCode, std::string> errorCauseCode2String;

		protected:
			/**
			 * Constructor is protected because we only want to create ErrorCause
			 * instances via Parse() and Factory() in subclasses.
			 */
			ErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ErrorCause() override;

			virtual void Dump(int indentation = 0) const override = 0;

			virtual ErrorCause* Clone(uint8_t* buffer, size_t bufferLength) const override = 0;

			virtual ErrorCauseCode GetCode() const final
			{
				return static_cast<ErrorCauseCode>(
				  uint16_t{ ntohs(static_cast<uint16_t>(GetHeaderPointer()->code)) });
			}

			/**
			 * False by default. UnknownErrorCause class overrides this method to
			 * return true instead.
			 */
			virtual bool HasUnknownCode() const
			{
				return false;
			}

		protected:
			/**
			 * Subclasses must invoke this method within their Dump() method.
			 */
			virtual void DumpCommon(int indentation) const override final;

			virtual void SoftSerialize(const uint8_t* buffer) final;

			virtual ErrorCause* SoftClone(const uint8_t* buffer) const = 0;

			virtual void SoftCloneInto(ErrorCause* errorCause) const final;

			virtual void InitializeHeader(ErrorCauseCode errorCauseCode, uint16_t lengthFieldValue) final;

			/**
			 * Error Cause subclasses with header bigger than default one (4 bytes)
			 * must override this method and return their header length (excluding
			 * variable-length field considered "value").
			 */
			virtual size_t GetHeaderLength() const override
			{
				return ErrorCause::ErrorCauseHeaderLength;
			}

		private:
			/**
			 * NOTE: Return ErrorCauseHeader* instead of const ErrorCauseHeader*
			 * since we may want to modify its fields.
			 */
			virtual ErrorCauseHeader* GetHeaderPointer() const final
			{
				return reinterpret_cast<ErrorCauseHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			virtual void SetCode(ErrorCauseCode causeCode) final
			{
				GetHeaderPointer()->code =
				  static_cast<ErrorCauseCode>(uint16_t{ htons(static_cast<uint16_t>(causeCode)) });
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
