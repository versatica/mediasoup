#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Chunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/chunkParameters/CookiePreservativeChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv4AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv6AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/UnknownChunkParameter.hpp"
#include "RTC/SCTP/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include "RTC/SCTP/errorCauses/MissingMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/errorCauses/UnknownErrorCause.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits()

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<Chunk::ChunkType, std::string> Chunk::chunkType2String =
		{
			{ Chunk::ChunkType::DATA,              "DATA"              },
			{ Chunk::ChunkType::INIT,              "INIT"              },
			{ Chunk::ChunkType::INIT_ACK,          "INIT_ACK"          },
			{ Chunk::ChunkType::SACK,              "SACK"              },
			{ Chunk::ChunkType::HEARTBEAT,         "HEARTBEAT"         },
			{ Chunk::ChunkType::HEARTBEAT_ACK,     "HEARTBEAT_ACK"     },
			{ Chunk::ChunkType::ABORT,             "ABORT"             },
			{ Chunk::ChunkType::SHUTDOWN,          "SHUTDOWN"          },
			{ Chunk::ChunkType::SHUTDOWN_ACK,      "SHUTDOWN_ACK"      },
			{ Chunk::ChunkType::OPERATION_ERROR,   "OPERATION_ERROR"   },
			{ Chunk::ChunkType::COOKIE_ECHO,       "COOKIE_ECHO"       },
			{ Chunk::ChunkType::COOKIE_ACK,        "COOKIE_ACK"        },
			{ Chunk::ChunkType::ECNE,              "ECNE"              },
			{ Chunk::ChunkType::CWR,               "CWR"               },
			{ Chunk::ChunkType::SHUTDOWN_COMPLETE, "SHUTDOWN_COMPLETE" }
			// TODO: Add more.
		};
		// clang-format on

		/* Class methods. */

		bool Chunk::IsChunk(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  ChunkType& chunkType,
		  uint16_t& chunkLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_WARN_TAG(sctp, "no space for SCTP Chunk Header [bufferLength:%zu]", bufferLength);

				return false;
			}

			const auto* chunkHeader = reinterpret_cast<const Chunk::ChunkHeader*>(buffer);

			chunkType   = chunkHeader->type;
			chunkLength = uint16_t{ ntohs(chunkHeader->length) };

			if (chunkLength < Chunk::ChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "Chunk Length field must have value greater or equal than %zu",
				  Chunk::ChunkHeaderLength);

				return false;
			}

			// Chunk total length must be multiple of 4 bytes and must include
			// padding bytes despite Chunk Length field doesn't not include padding.
			// NOTE: We must cast to size_t, otherwise a maximum Chunk Length value
			// of 65535 would generate a padded length of 0 bytes!
			size_t paddedChunkLength = Utils::Byte::PadTo4Bytes(size_t{ chunkLength });

			if (bufferLength < paddedChunkLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "no space for 4-byte padded announced Chunk Length [paddedChunkLength:%zu, bufferLength:%zu]",
				  paddedChunkLength,
				  bufferLength);

				return false;
			}

			padding = paddedChunkLength - chunkLength;

			return true;
		}

		const std::string& Chunk::ChunkType2String(ChunkType chunkType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Chunk::chunkType2String.find(chunkType);

			if (it == Chunk::chunkType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		Chunk::Chunk(uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Chunk::~Chunk()
		{
			MS_TRACE();

			if (CanHaveParameters())
			{
				for (const auto* parameter : this->parameters)
				{
					delete parameter;
				}
			}

			if (CanHaveErrorCauses())
			{
				for (const auto* errorCause : this->errorCauses)
				{
					delete errorCause;
				}
			}
		}

		void Chunk::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			if (CanHaveParameters())
			{
				for (auto* parameter : this->parameters)
				{
					size_t offset = parameter->GetBuffer() - previousBuffer;

					parameter->SoftSerialize(buffer + offset);
				}
			}

			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					size_t offset = errorCause->GetBuffer() - previousBuffer;

					errorCause->SoftSerialize(buffer + offset);
				}
			}
		}

		void Chunk::AddParameter(const ChunkParameter* parameter)
		{
			MS_TRACE();

			AssertNotFrozen();
			AssertCanHaveParameters();

			size_t length = GetLength() + parameter->GetLength();

			// Let's append the Parameter at the end of existing Parameters.
			auto* clonedParameter =
			  parameter->Clone(const_cast<uint8_t*>(GetBuffer()) + GetLength(), parameter->GetLength());

			// Update Serializable length.
			try
			{
				SetLength(length);
			}
			catch (const MediaSoupError& error)
			{
				delete clonedParameter;

				throw;
			}

			// Freeze the cloned Parameter.
			clonedParameter->Freeze();

			this->parameters.push_back(clonedParameter);
		}

		void Chunk::AddErrorCause(const ErrorCause* errorCause)
		{
			MS_TRACE();

			AssertNotFrozen();
			AssertCanHaveErrorCauses();

			size_t length = GetLength() + errorCause->GetLength();

			// Let's append the Error Cause at the end of existing Error Causes.
			auto* clonedErrorCause =
			  errorCause->Clone(const_cast<uint8_t*>(GetBuffer()) + GetLength(), errorCause->GetLength());

			// Update Serializable length.
			try
			{
				SetLength(length);
			}
			catch (const MediaSoupError& error)
			{
				delete clonedErrorCause;

				throw;
			}

			// Freeze the cloned Error Cause.
			clonedErrorCause->Freeze();

			this->errorCauses.push_back(clonedErrorCause);
		}

		void Chunk::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation, "  length + padding: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu8 " (%s) (unknown: %s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  flags: " MS_UINT8_TO_BINARY_PATTERN, MS_UINT8_TO_BINARY(GetFlags()));
			MS_DUMP_CLEAN(indentation, "  length field: %" PRIu16, GetLengthField());
		}

		void Chunk::DumpParameters(int indentation) const
		{
			MS_TRACE();

			if (CanHaveParameters())
			{
				MS_DUMP_CLEAN(indentation, "  parameters count: %zu", GetParametersCount());
				for (const auto* parameter : this->parameters)
				{
					parameter->Dump(indentation + 1);
				}
			}
		}

		void Chunk::DumpErrorCauses(int indentation) const
		{
			MS_TRACE();

			if (CanHaveErrorCauses())
			{
				MS_DUMP_CLEAN(indentation, "  error causes count: %zu", GetErrorCausesCount());
				for (const auto* errorCause : this->errorCauses)
				{
					errorCause->Dump(indentation + 1);
				}
			}
		}

		void Chunk::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			SetBuffer(const_cast<uint8_t*>(buffer));

			if (CanHaveParameters())
			{
				for (auto* parameter : this->parameters)
				{
					size_t offset = parameter->GetBuffer() - previousBuffer;

					parameter->SoftSerialize(buffer + offset);
				}
			}

			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					size_t offset = errorCause->GetBuffer() - previousBuffer;

					errorCause->SoftSerialize(buffer + offset);
				}
			}
		}

		void Chunk::SoftCloneInto(Chunk* chunk) const
		{
			MS_TRACE();

			// Soft clone Chunk Parameters into the given Chunk.
			if (CanHaveParameters())
			{
				for (auto* parameter : this->parameters)
				{
					size_t offset = parameter->GetBuffer() - GetBuffer();

					auto* softClonedParameter = parameter->SoftClone(chunk->GetBuffer() + offset);

					// ChunkParameter constructors don't freeze the ChunkParameter so we
					// must do it manually.
					softClonedParameter->Freeze();

					chunk->parameters.push_back(softClonedParameter);
				}
			}

			// Soft clone Error Causes into the given Chunk.
			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					size_t offset = errorCause->GetBuffer() - GetBuffer();

					auto* softClonedErrorCause = errorCause->SoftClone(chunk->GetBuffer() + offset);

					// ErrorCause constructors don't freeze the ErrorCause so we must do
					// it manually.
					softClonedErrorCause->Freeze();

					chunk->errorCauses.push_back(softClonedErrorCause);
				}
			}

			// Need to manually set Serializable length.
			chunk->SetLength(GetLength());
		}

		void Chunk::InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetType(chunkType);
			SetFlags(flags);
			SetLengthField(lengthFieldValue);
		}

		void Chunk::SetValue(const uint8_t* value, size_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This can throw.
			SetValueLength(valueLength);

			// Copy the given value into the buffer.
			std::memmove(GetValuePointer(), value, valueLength);
		}

		void Chunk::SetValueLength(size_t valueLength)
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
				// Let's call SetLength() on parent with the new computed Chunk length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				// NOTE: Chunks must be padded to 4 bytes.
				SetLength(newPaddedLength);

				// Update the Chunk Length field.
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

		bool Chunk::ParseParameters()
		{
			MS_TRACE();

			AssertCanHaveParameters();

			// Here we assume that the Chunk buffer has been validated and
			// GetLength() returns the fixed minimum length of the specific Chunk
			// subclass, so GetBuffer() + GetLength() points to the beginning of
			// the potential Chunk Parameters.
			// And of course we assume that a Chunk cannot have both Chunk Parameters
			// and Error Causes.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();

			// Here we assume that the Chunk has been validated so Length field is
			// reliable. We want to be ready for Length field to include or not the
			// possible padding of the last Chunk Parameter (as per RFC
			// recommendation). In fact, we rely on parameter->GetLength() while
			// parsing the buffer so we want to provide each
			// ChunkParameter::StrictParse() call with a 4-bytes padded buffer length.
			const auto* end = GetBuffer() + Utils::Byte::PadTo4Bytes(GetLengthField());

			while (ptr < end)
			{
				// The remaining length in the given length is the potential buffer
				// length of the Chunk Parameter.
				size_t parameterMaxBufferLength = end - ptr;

				// Here we must anticipate the type of each Parameter to use its
				// appropriate parser.
				ChunkParameter::ChunkParameterType parameterType;
				uint16_t parameterLength;
				uint8_t padding;

				if (!ChunkParameter::IsChunkParameter(
				      ptr, parameterMaxBufferLength, parameterType, parameterLength, padding))
				{
					MS_WARN_TAG(sctp, "not a SCTP Chunk Parameter");

					return false;
				}

				ChunkParameter* parameter{ nullptr };

				// TODO: Add more.
				switch (parameterType)
				{
					case ChunkParameter::ChunkParameterType::HEARTBEAT_INFO:
					{
						parameter = HeartbeatInfoChunkParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case ChunkParameter::ChunkParameterType::IPV4_ADDRESS:
					{
						parameter = IPv4AddressChunkParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case ChunkParameter::ChunkParameterType::IPV6_ADDRESS:
					{
						parameter = IPv6AddressChunkParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE:
					{
						parameter = CookiePreservativeChunkParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					default:
					{
						parameter = UnknownChunkParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);
					}
				}

				if (!parameter)
				{
					return false;
				}

				this->parameters.push_back(parameter);

				ptr += parameter->GetLength();
			}

			if (ptr != end)
			{
				auto expectedLength = end - GetBuffer();
				auto computedLength = ptr - GetBuffer();

				MS_WARN_TAG(
				  sctp,
				  "computed length (%zu bytes) doesn't match the expected length (%zu bytes)",
				  computedLength,
				  expectedLength);

				return false;
			}

			return true;
		}

		bool Chunk::ParseErrorCauses()
		{
			MS_TRACE();

			AssertCanHaveErrorCauses();

			// Here we assume that the Chunk buffer has been validated and
			// GetLength() returns the fixed minimum length of the specific Chunk
			// subclass, so GetBuffer() + GetLength() points to the beginning of
			// the potential Error Causes.
			// And of course we assume that a Chunk cannot have both Chunk Parameters
			// and Error Causes.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();

			// Here we assume that the Chunk has been validated so Length field is
			// reliable. We want to be ready for Length field to include or not the
			// possible padding of the last Error Cause (as per RFCrecommendation).
			// In fact, we rely on errorCause->GetLength() while parsing the buffer
			// so we want to provide each ErrorCause::StrictParse() call with a
			// 4-bytes padded buffer length.
			const auto* end = GetBuffer() + Utils::Byte::PadTo4Bytes(GetLengthField());

			while (ptr < end)
			{
				// The remaining length in the given length is the potential buffer
				// length of the Error Cause.
				size_t errorCauseMaxBufferLength = end - ptr;

				// Here we must anticipate the type of each Error Cause to use its
				// appropriate parser.
				ErrorCause::ErrorCauseCode causeCode;
				uint16_t causeLength;
				uint8_t padding;

				if (!ErrorCause::IsErrorCause(ptr, errorCauseMaxBufferLength, causeCode, causeLength, padding))
				{
					MS_WARN_TAG(sctp, "not a SCTP Error Cause");

					return false;
				}

				ErrorCause* errorCause{ nullptr };

				// TODO: Add more.
				switch (causeCode)
				{
					case ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER:
					{
						errorCause = InvalidStreamIdentifierErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER:
					{
						errorCause = MissingMandatoryParameterErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::STALE_COOKIE:
					{
						errorCause =
						  StaleCookieErrorCause::ParseStrict(ptr, causeLength + padding, causeLength, padding);

						break;
					}

					default:
					{
						errorCause =
						  UnknownErrorCause::ParseStrict(ptr, causeLength + padding, causeLength, padding);
					}
				}

				if (!errorCause)
				{
					return false;
				}

				this->errorCauses.push_back(errorCause);

				ptr += errorCause->GetLength();
			}

			if (ptr != end)
			{
				auto expectedLength = end - GetBuffer();
				auto computedLength = ptr - GetBuffer();

				MS_WARN_TAG(
				  sctp,
				  "computed length (%zu bytes) doesn't match the expected length (%zu bytes)",
				  computedLength,
				  expectedLength);

				return false;
			}

			return true;
		}

		void Chunk::SetLengthField(size_t length)
		{
			MS_TRACE();

			if (length > std::numeric_limits<uint16_t>::max())
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) cannot be greater than 65535", length);
			}

			GetHeaderPointer()->length = uint16_t{ htons(length) };
		}

		void Chunk::HandleInPlaceParameter(ChunkParameter* parameter)
		{
			MS_TRACE();

			// When the application completes the Parameter it must call
			// `parameter->Consolidate()` and that will trigger this event.
			parameter->SetConsolidatedListener(
			  [this, parameter]()
			  {
				  // Fix buffer length assigned to the Parameter.
				  parameter->SetBufferLength(parameter->GetLength());

				  // Freeze the Parameter.
				  parameter->Freeze();

				  auto previousLength      = GetLength();
				  auto previousLengthField = GetLengthField();

				  try
				  {
					  // Update Chunk length.
					  // NOTE: This will throw if there is no enough space in the Chunk
					  // buffer.
					  SetLength(previousLength + parameter->GetLength());

					  // Here we have to update the Chunk Value Length and this is not
					  // easy because we have to take into account the padding of all
					  // Parameters but the last one. So we do this:
					  // - We assume that Parameters are always at the end of the Chunk.
					  // - We read the Parameter Length field of the new added Parameter.
					  // - We add it to the previous total length of the Chunk and set
					  //   the Chunk Length field with the resulting value.
					  //
					  // NOTE: This will throw if computed Length field value is too big.
					  SetLengthField(previousLength + parameter->GetLengthField());
				  }
				  catch (const MediaSoupError& error)
				  {
					  // Rollback.
					  SetLength(previousLength);
					  SetLengthField(previousLengthField);

					  throw;
				  }

				  // Add the Parameter to the list.
				  this->parameters.push_back(parameter);
			  });
		}

		void Chunk::HandleInPlaceErrorCause(ErrorCause* errorCause)
		{
			MS_TRACE();

			// When the application completes the Error Cause it must call
			// `errorCause->Consolidate()` and that will trigger this event.
			errorCause->SetConsolidatedListener(
			  [this, errorCause]()
			  {
				  // Fix buffer length assigned to the Error Cause.
				  errorCause->SetBufferLength(errorCause->GetLength());

				  // Freeze the Error Cause.
				  errorCause->Freeze();

				  auto previousLength      = GetLength();
				  auto previousLengthField = GetLengthField();

				  try
				  {
					  // Update Chunk length.
					  // NOTE: This will throw if there is no enough space in the Chunk
					  // buffer.
					  SetLength(previousLength + errorCause->GetLength());

					  // Here we have to update the Chunk Value Length and this is not
					  // easy because we have to take into account the padding of all
					  // Error Causes but the last one. So we do this:
					  // - We assume that Error Causes are always at the end of the
					  //   Chunk.
					  // - We read the Error Cause Length field of the new added Error
					  //   Cause.
					  // - We add it to the previous total length of the Chunk and set
					  //   the Chunk Length field with the resulting value.
					  //
					  // NOTE: This will throw if computed Length field value is too big.
					  SetLengthField(previousLength + errorCause->GetLengthField());
				  }
				  catch (const MediaSoupError& error)
				  {
					  // Rollback.
					  SetLength(previousLength);
					  SetLengthField(previousLengthField);

					  throw;
				  }

				  // Add the Error Cause to the list.
				  this->errorCauses.push_back(errorCause);
			  });
		}

		void Chunk::AssertCanHaveParameters() const
		{
			MS_TRACE();

			if (!CanHaveParameters())
			{
				MS_THROW_ERROR("this Chunk class cannot have Chunk Parameters");
			}
		}

		void Chunk::AssertCanHaveErrorCauses() const
		{
			MS_TRACE();

			if (!CanHaveErrorCauses())
			{
				MS_THROW_ERROR("this Chunk class cannot have Error Causes");
			}
		}
	} // namespace SCTP
} // namespace RTC
