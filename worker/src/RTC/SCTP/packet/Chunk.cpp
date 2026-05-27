#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/Chunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/MissingMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/RestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnresolvableAddressErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/AddIncomingStreamsRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/AddOutgoingStreamsRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/packet/parameters/SsnTsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedAddressTypesParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"
#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		const std::unordered_map<Chunk::ChunkType, std::string> Chunk::ChunkType2String =
		{
			{ Chunk::ChunkType::DATA,              "DATA"              },
			{ Chunk::ChunkType::INIT,              "INIT"              },
			{ Chunk::ChunkType::INIT_ACK,          "INIT-ACK"          },
			{ Chunk::ChunkType::SACK,              "SACK"              },
			{ Chunk::ChunkType::HEARTBEAT_REQUEST, "HEARTBEAT-REQUEST" },
			{ Chunk::ChunkType::HEARTBEAT_ACK,     "HEARTBEAT-ACK"     },
			{ Chunk::ChunkType::ABORT,             "ABORT"             },
			{ Chunk::ChunkType::SHUTDOWN,          "SHUTDOWN"          },
			{ Chunk::ChunkType::SHUTDOWN_ACK,      "SHUTDOWN-ACK"      },
			{ Chunk::ChunkType::OPERATION_ERROR,   "OPERATION-ERROR"   },
			{ Chunk::ChunkType::COOKIE_ECHO,       "COOKIE-ECHO"       },
			{ Chunk::ChunkType::COOKIE_ACK,        "COOKIE-ACK"        },
			{ Chunk::ChunkType::ECNE,              "ECNE"              },
			{ Chunk::ChunkType::CWR,               "CWR"               },
			{ Chunk::ChunkType::SHUTDOWN_COMPLETE, "SHUTDOWN-COMPLETE" },
			{ Chunk::ChunkType::FORWARD_TSN,       "FORWARD-TSN"       },
			{ Chunk::ChunkType::RE_CONFIG,         "RE-CONFIG"         },
			{ Chunk::ChunkType::I_DATA,            "I-DATA"            },
			{ Chunk::ChunkType::I_FORWARD_TSN,     "I-FORWARD-TSN"     },
		};
		// clang-format on

		/* Class methods. */

		bool Chunk::IsChunk(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  Chunk::ChunkType& chunkType,
		  uint16_t& chunkLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (!TLV::IsTLV(buffer, bufferLength, chunkLength, padding))
			{
				return false;
			}

			chunkType = static_cast<Chunk::ChunkType>(buffer[0]);

			return true;
		}

		const std::string& Chunk::ChunkTypeToString(ChunkType chunkType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Chunk::ChunkType2String.find(chunkType);

			if (it == Chunk::ChunkType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		Chunk::Chunk(uint8_t* buffer, size_t bufferLength) : TLV(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Chunk::~Chunk()
		{
			MS_TRACE();

			// NOTE: Here we cannot check CanHaveParameters() or CanHaveErrorCauses()
			// because this is the destructor of chunk so the subclass has been
			// already destroyed (its destructor runs first).

			for (const auto* parameter : this->parameters)
			{
				delete parameter;
			}

			for (const auto* errorCause : this->errorCauses)
			{
				delete errorCause;
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
					const size_t offset = parameter->GetBuffer() - previousBuffer;

					parameter->SoftSerialize(buffer + offset);
				}
			}

			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					const size_t offset = errorCause->GetBuffer() - previousBuffer;

					errorCause->SoftSerialize(buffer + offset);
				}
			}
		}

		void Chunk::AddParameter(const Parameter* parameter)
		{
			MS_TRACE();

			AssertCanHaveParameters();
			AssertDoesNotNeedConsolidation();

			const size_t previousLength = GetLength();

			// This will update the total length and length field of the chunk.
			// NOTE: It may throw.
			AddItem(parameter);

			// Let's append the parameter at the end of existing parameters.
			auto* clonedParameter =
			  parameter->Clone(const_cast<uint8_t*>(GetBuffer()) + previousLength, parameter->GetLength());

			// Add the parameter to the list.
			this->parameters.push_back(clonedParameter);
		}

		void Chunk::AddErrorCause(const ErrorCause* errorCause)
		{
			MS_TRACE();

			AssertCanHaveErrorCauses();
			AssertDoesNotNeedConsolidation();

			const size_t previousLength = GetLength();

			// This will update the total length and length field of the chunk.
			// NOTE: It may throw.
			AddItem(errorCause);

			// Let's append the error cause at the end of existing error causes.
			auto* clonedErrorCause = errorCause->Clone(
			  const_cast<uint8_t*>(GetBuffer()) + previousLength, errorCause->GetLength());

			this->errorCauses.push_back(clonedErrorCause);
		}

		void Chunk::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu8 " (%s) (unknown: %s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkTypeToString(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  flags: " MS_UINT8_TO_BINARY_PATTERN, MS_UINT8_TO_BINARY(GetFlags()));
			TLV::DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  needs consolidation of parameters or error causes: %s",
			  NeedsConsolidation() ? "yes" : "no");
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
					const size_t offset = parameter->GetBuffer() - previousBuffer;

					parameter->SoftSerialize(buffer + offset);
				}
			}

			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					const size_t offset = errorCause->GetBuffer() - previousBuffer;

					errorCause->SoftSerialize(buffer + offset);
				}
			}
		}

		void Chunk::SoftCloneInto(Chunk* chunk) const
		{
			MS_TRACE();

			// Soft clone parameters into the given chunk.
			if (CanHaveParameters())
			{
				for (auto* parameter : this->parameters)
				{
					const size_t offset = parameter->GetBuffer() - GetBuffer();

					auto* softClonedParameter = parameter->SoftClone(chunk->GetBuffer() + offset);

					chunk->parameters.push_back(softClonedParameter);
				}
			}

			// Soft clone error causes into the given chunk.
			if (CanHaveErrorCauses())
			{
				for (auto* errorCause : this->errorCauses)
				{
					const size_t offset = errorCause->GetBuffer() - GetBuffer();

					auto* softClonedErrorCause = errorCause->SoftClone(chunk->GetBuffer() + offset);

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
			InitializeTLVHeader(lengthFieldValue);
		}

		bool Chunk::ParseParameters()
		{
			MS_TRACE();

			AssertCanHaveParameters();

			// Here we assume that the chunk buffer has been validated and
			// GetLength() returns the fixed minimum length of the specific chunk
			// subclass, so GetBuffer() + GetLength() points to the beginning of the
			// potential parameters. And of course we assume that a chunk cannot have
			// both parameters and error causes.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();

			// Here we assume that the chunk has been validated so length field is
			// reliable. We want to be ready for length field to include or not the
			// possible padding of the last parameter (as per RFC recommendation). In
			// fact, we rely on parameter->GetLength() while parsing the buffer so we
			// want to provide each Parameter::StrictParse() call with a 4-bytes
			// padded buffer length.
			const auto* end = GetBuffer() + Utils::Byte::PadTo4Bytes(GetLengthField());

			while (ptr < end)
			{
				// The remaining length in the given length is the potential buffer
				// length of the parameter.
				const size_t parameterMaxBufferLength = end - ptr;

				// Here we must anticipate the type of each parameter to use its
				// appropriate parser.
				Parameter::ParameterType parameterType;
				uint16_t parameterLength;
				uint8_t padding;

				if (!Parameter::IsParameter(
				      ptr, parameterMaxBufferLength, parameterType, parameterLength, padding))
				{
					MS_WARN_TAG(sctp, "not an SCTP parameter");

					return false;
				}

				Parameter* parameter{ nullptr }; // NOLINT(misc-const-correctness)

				switch (parameterType)
				{
					case Parameter::ParameterType::HEARTBEAT_INFO:
					{
						parameter = HeartbeatInfoParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::IPV4_ADDRESS:
					{
						parameter = IPv4AddressParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::IPV6_ADDRESS:
					{
						parameter = IPv6AddressParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::STATE_COOKIE:
					{
						parameter = StateCookieParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::UNRECOGNIZED_PARAMETER:
					{
						parameter = UnrecognizedParameterParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::COOKIE_PRESERVATIVE:
					{
						parameter = CookiePreservativeParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES:
					{
						parameter = SupportedAddressTypesParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::FORWARD_TSN_SUPPORTED:
					{
						parameter = ForwardTsnSupportedParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::SUPPORTED_EXTENSIONS:
					{
						parameter = SupportedExtensionsParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE:
					{
						parameter = ZeroChecksumAcceptableParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST:
					{
						parameter = OutgoingSsnResetRequestParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST:
					{
						parameter = IncomingSsnResetRequestParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::SSN_TSN_RESET_REQUEST:
					{
						parameter = SsnTsnResetRequestParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::RECONFIGURATION_RESPONSE:
					{
						parameter = ReconfigurationResponseParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST:
					{
						parameter = AddOutgoingStreamsRequestParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					case Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST:
					{
						parameter = AddIncomingStreamsRequestParameter::ParseStrict(
						  ptr, parameterLength + padding, parameterLength, padding);

						break;
					}

					default:
					{
						parameter = UnknownParameter::ParseStrict(
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

			// Here we assume that the chunk buffer has been validated and GetLength()
			// returns the fixed minimum length of the specific chunk subclass, so
			// GetBuffer() + GetLength() points to the beginning of the potential
			// error causes. And of course we assume that a chunk cannot have both
			// parameters and error causes.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();

			// Here we assume that the chunk has been validated so length field is
			// reliable. We want to be ready for length field to include or not the
			// possible padding of the last error cause (as per RFCrecommendation).
			// In fact, we rely on errorCause->GetLength() while parsing the buffer
			// so we want to provide each ErrorCause::StrictParse() call with a
			// 4-bytes padded buffer length.
			const auto* end = GetBuffer() + Utils::Byte::PadTo4Bytes(GetLengthField());

			while (ptr < end)
			{
				// The remaining length in the given length is the potential buffer
				// length of the error cause.
				const size_t errorCauseMaxBufferLength = end - ptr;

				// Here we must anticipate the type of each error cause to use its
				// appropriate parser.
				ErrorCause::ErrorCauseCode causeCode;
				uint16_t causeLength;
				uint8_t padding;

				if (!ErrorCause::IsErrorCause(ptr, errorCauseMaxBufferLength, causeCode, causeLength, padding))
				{
					MS_WARN_TAG(sctp, "not an SCTP error cause");

					return false;
				}

				ErrorCause* errorCause{ nullptr }; // NOLINT(misc-const-correctness)

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

					case ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE:
					{
						errorCause =
						  OutOfResourceErrorCause::ParseStrict(ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS:
					{
						errorCause = UnresolvableAddressErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE:
					{
						errorCause = UnrecognizedChunkTypeErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::INVALID_MANDATORY_PARAMETER:
					{
						errorCause = InvalidMandatoryParameterErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS:
					{
						errorCause = UnrecognizedParametersErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::NO_USER_DATA:
					{
						errorCause =
						  NoUserDataErrorCause::ParseStrict(ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN:
					{
						errorCause = CookieReceivedWhileShuttingDownErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES:
					{
						errorCause = RestartOfAnAssociationWithNewAddressesErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT:
					{
						errorCause = UserInitiatedAbortErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

						break;
					}

					case ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION:
					{
						errorCause = ProtocolViolationErrorCause::ParseStrict(
						  ptr, causeLength + padding, causeLength, padding);

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

		void Chunk::HandleInPlaceParameter(Parameter* parameter)
		{
			MS_TRACE();

			this->needsConsolidation = true;

			// When the application completes the parameter it must call
			// `parameter->Consolidate()` and that will trigger this event.
			parameter->SetConsolidatedListener(
			  [this, parameter]()
			  {
				  try
				  {
					  // Fix buffer length assigned to the parameter.
					  // NOTE: It may throw.
					  parameter->SetBufferLength(parameter->GetLength());

					  // This will update the total length and length field of the chunk.
					  // NOTE: It may throw.
					  AddItem(parameter);

					  // Add the parameter to the list.
					  this->parameters.push_back(parameter);

					  this->needsConsolidation = false;
				  }
				  catch (const MediaSoupError& error)
				  {
					  this->needsConsolidation = false;

					  throw;
				  }
			  });
		}

		void Chunk::HandleInPlaceErrorCause(ErrorCause* errorCause)
		{
			MS_TRACE();

			this->needsConsolidation = true;

			// When the application completes the error cause it must call
			// `errorCause->Consolidate()` and that will trigger this event.
			errorCause->SetConsolidatedListener(
			  [this, errorCause]()
			  {
				  try
				  {
					  // Fix buffer length assigned to the error cause.
					  errorCause->SetBufferLength(errorCause->GetLength());

					  // This will update the total length and length field of the chunk.
					  // NOTE: It may throw.
					  AddItem(errorCause);

					  // Add the error cause to the list.
					  this->errorCauses.push_back(errorCause);

					  this->needsConsolidation = false;
				  }
				  catch (const MediaSoupError& error)
				  {
					  this->needsConsolidation = false;

					  throw;
				  }
			  });
		}

		void Chunk::AssertCanHaveParameters() const
		{
			MS_TRACE();

			if (!CanHaveParameters())
			{
				MS_THROW_ERROR("this chunk class cannot have parameters");
			}
		}

		void Chunk::AssertCanHaveErrorCauses() const
		{
			MS_TRACE();

			if (!CanHaveErrorCauses())
			{
				MS_THROW_ERROR("this chunk class cannot have error causes");
			}
		}

		void Chunk::AssertDoesNotNeedConsolidation() const
		{
			MS_TRACE();

			if (this->needsConsolidation)
			{
				MS_THROW_ERROR("chunk needs consolidation of some ongoing parameter or error cause");
			}
		}
	} // namespace SCTP
} // namespace RTC
