#define MS_CLASS "RTC::SCTP::ErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/ErrorCause.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits()

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ErrorCause::ErrorCauseCode, std::string> ErrorCause::errorCauseCode2String =
		{
			{ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,   "INVALID_STREAM_IDENTIFIER"   },
			{ ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER, "MISSING_MANDATORY_PARAMETER" },
			{ ErrorCause::ErrorCauseCode::STALE_COOKIE,                "STALE_COOKIE"                },
			{ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,             "OUT_OF_RESOURCE"             },
			{ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,        "UNRESOLVABLE_ADDRESS"        },
			{ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,     "UNRECOGNIZED_CHUNK_TYPE"     },
			// TODO: Add more.
		};
		// clang-format on

		/* Class methods. */

		bool ErrorCause::IsErrorCause(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  ErrorCauseCode& causeCode,
		  uint16_t& causeLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (bufferLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_WARN_TAG(sctp, "no space for SCTP Error Cause Header [bufferLength:%zu]", bufferLength);

				return false;
			}

			const auto* causeHeader = reinterpret_cast<const ErrorCause::ErrorCauseHeader*>(buffer);

			causeCode =
			  static_cast<ErrorCauseCode>(uint16_t{ ntohs(static_cast<uint16_t>(causeHeader->code)) });
			causeLength = uint16_t{ ntohs(causeHeader->length) };

			if (causeLength < ErrorCause::ErrorCauseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "Error Cause Length field must have value greater or equal than %zu",
				  ErrorCause::ErrorCauseHeaderLength);

				return false;
			}

			// Error Cause total length must be multiple of 4 bytes and must include
			// padding bytes despite Cause Length field doesn't not include padding.
			// NOTE: We must cast to size_t, otherwise a maximum Cause Length value
			// of 65535 would generate a padded length of 0 bytes!
			size_t paddedCauseLength = Utils::Byte::PadTo4Bytes(size_t{ causeLength });

			if (bufferLength < paddedCauseLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "no space for 4-byte padded announced Error Cause Length [paddedCauseLength:%zu, bufferLength:%zu]",
				  paddedCauseLength,
				  bufferLength);

				return false;
			}

			padding = paddedCauseLength - causeLength;

			return true;
		}

		const std::string& ErrorCause::ErrorCauseCode2String(ErrorCauseCode causeCode)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ErrorCause::errorCauseCode2String.find(causeCode);

			if (it == ErrorCause::errorCauseCode2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		ErrorCause::ErrorCause(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		ErrorCause::~ErrorCause()
		{
			MS_TRACE();
		}

		void ErrorCause::SoftCloneInto(ErrorCause* errorCause) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			errorCause->SetLength(GetLength());
		}

		void ErrorCause::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation, "  length + padding: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(
			  indentation,
			  "  code: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetCode()),
			  ErrorCause::ErrorCauseCode2String(GetCode()).c_str(),
			  HasUnknownCode() ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")",
			  GetLengthField(),
			  GetValueLength());
		}

		void ErrorCause::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void ErrorCause::InitializeHeader(ErrorCauseCode causeCode, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetCode(causeCode);
			SetLengthField(lengthFieldValue);
		}

		void ErrorCause::SetLengthField(size_t length)
		{
			MS_TRACE();

			if (length > std::numeric_limits<uint16_t>::max())
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) cannot be greater than 65535", length);
			}

			GetHeaderPointer()->length = uint16_t{ htons(length) };
		}

		void ErrorCause::SetValue(const uint8_t* value, size_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This can throw.
			SetValueLength(valueLength);

			// Copy the given value into the buffer.
			std::memmove(GetValuePointer(), value, valueLength);
		}

		void ErrorCause::SetValueLength(size_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength      = GetLength();
			auto previousLengthField = GetLengthField();
			auto previousValueLength = GetValueLength();
			auto newNotPaddedLength =
			  size_t{ previousLengthField } - size_t{ previousValueLength } + valueLength;
			auto newPaddedLength = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			try
			{
				// Let's call SetLength() on parent with the new computed Error Cause
				// length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				// NOTE: Error Causes must be padded to 4 bytes.
				SetLength(newPaddedLength);

				// Update the Error Cause Length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newNotPaddedLength);
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}
	} // namespace SCTP
} // namespace RTC
