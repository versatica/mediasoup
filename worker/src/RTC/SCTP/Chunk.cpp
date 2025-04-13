#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Chunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/UnknownChunkParameter.hpp"
#include <cstring> // std::memmove()

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
			{ Chunk::ChunkType::ERROR,             "ERROR"             },
			{ Chunk::ChunkType::COOKIE_ECHO,       "COOKIE_ECHO"       },
			{ Chunk::ChunkType::COOKIE_ACK,        "COOKIE_ACK"        },
			{ Chunk::ChunkType::ECNE,              "ECNE"              },
			{ Chunk::ChunkType::CWR,               "CWR"               },
			{ Chunk::ChunkType::SHUTDOWN_COMPLETE, "SHUTDOWN_COMPLETE" }
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

		Chunk::Chunk(const uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Chunk::~Chunk()
		{
			MS_TRACE();

			for (auto* parameter : this->parameters)
			{
				delete parameter;
			}
		}

		void Chunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<Chunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
			MS_DUMP(
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")", GetLengthField(), GetValueLength());
			MS_DUMP("  has parameters: %s", HasParameters() ? "yes" : "no");
			MS_DUMP("  parameters count: %zu", GetParametersCount());
			for (auto* parameter : this->parameters)
			{
				parameter->Dump();
			}
			MS_DUMP("</Chunk>");
		}

		void Chunk::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			// Reassign pointers.
			for (auto* parameter : this->parameters)
			{
				size_t offset = parameter->GetBuffer() - previousBuffer;

				parameter->SetBuffer(buffer + offset);
			}
		}

		Chunk* Chunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new Chunk(buffer, bufferLength);

			CloneInto(clonedChunk);

			return clonedChunk;
		}

		void Chunk::AddParameter(const ChunkParameter* parameter)
		{
			MS_TRACE();

			AssertNotFrozen();

			size_t length = GetLength() + parameter->GetLength();

			// Let's append the Parameter at the end of existing Parameters.
			auto* clonedParameter =
			  parameter->Clone(const_cast<uint8_t*>(GetBuffer()) + GetLength(), parameter->GetLength());

			// Freeze the cloned Parameter.
			clonedParameter->Freeze();

			this->parameters.push_back(clonedParameter);

			// Update Serializable length.
			SetLength(length);
		}

		ChunkParameter* Chunk::BuildParameterInPlace(ChunkParameter::ChunkParameterType parameterType)
		{
			MS_TRACE();

			ChunkParameter* parameter{ nullptr };

			// The new Parameter will be added after other Parameters in the Chunk,
			// this is, at the end of the Chunk.
			auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
			// The remaining length in the buffer is the potential buffer length
			// of the Parameter.
			size_t parameterMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

			// TODO
			switch (parameterType)
			{
				case ChunkParameter::ChunkParameterType::HEARTBEAT_INFO:
				{
					parameter = HeartbeatInfoChunkParameter::Factory(ptr, parameterMaxBufferLength);

					break;
				}
			}

			// NOTE: Do not fix/update the Parameter buffer length since the caller
			// probably wants to modify the Parameter.

			// When the application completes the Parameter it must call
			// `parameter->Consolidate()` and that will trigger this event.
			parameter->SetConsolidatedListener(
			  [this, parameter]()
			  {
				  // Fix buffer length assigned to the Parameter.
				  parameter->SetBufferLength(parameter->GetLength());

				  // NOTE: No need to freeze the Parameter because `Consolidate()` did
				  // it.

				  auto previousLength = GetLength();

				  // Add the Parameter to the list.
				  this->parameters.push_back(parameter);

				  // Update Chunk length.
				  SetLength(previousLength + parameter->GetLength());

				  // Here we have to update the Chunk Value Length and this is not easy
				  // because we have to take into account the padding of all Parameters
				  // but the last one. So we do this:
				  // - We assume that Parameters are always at the end of the Chunk.
				  // - We read the Parameter Length field of the new added Parameter.
				  // - We add it to the previous total length of the Chunk and
				  //   set the Chunk Length field with the resulting value.
				  SetLengthField(previousLength + parameter->GetLengthField());
			  });

			return parameter;
		}

		void Chunk::CloneInto(Serializable* serializable) const
		{
			MS_TRACE();

			auto* chunk = static_cast<Chunk*>(serializable);

			Serializable::CloneInto(chunk);

			// Add a new parsed ChunkParameter for each ChunkParameter in this Chunk
			// and make it point to its position in the new buffer.
			for (const auto* parameter : this->parameters)
			{
				size_t offset = parameter->GetBuffer() - GetBuffer();

				ChunkParameter* clonedParameter{ nullptr };

				// TODO
				switch (parameter->GetType())
				{
					case ChunkParameter::ChunkParameterType::HEARTBEAT_INFO:
					{
						clonedParameter =
						  new HeartbeatInfoChunkParameter(chunk->GetBuffer() + offset, parameter->GetLength());

						break;
					}

					default:
					{
						clonedParameter =
						  new UnknownChunkParameter(chunk->GetBuffer() + offset, parameter->GetLength());
					}
				}

				// Set the proper ChunkParameter length.
				// NOTE: This should not throw but just in case.
				try
				{
					clonedParameter->SetLength(parameter->GetLength());
				}
				catch (const MediaSoupError& error)
				{
					delete chunk;
					delete clonedParameter;

					throw;
				}

				// ChunkParameter constructors don't freeze the Chunk so we must do it
				// manually.
				clonedParameter->Freeze();

				chunk->parameters.push_back(clonedParameter);
			}
		}

		void Chunk::InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetType(chunkType);
			SetFlags(flags);
			SetLengthField(lengthFieldValue);
		}

		bool Chunk::ParseParameters(const uint8_t* buffer, uint16_t bufferLength)
		{
			MS_TRACE();

			auto* ptr = const_cast<uint8_t*>(buffer);

			while (ptr < buffer + bufferLength)
			{
				// The remaining length in the given length is the potential buffer
				// length of the Chunk Parameter.
				size_t parameterMaxBufferLength = bufferLength - (ptr - buffer);

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

				MS_DEBUG_DEV(
				  "parsing SCTP Chunk Parameter [ptr:%zu, type:%" PRIu16 "]", ptr - buffer, parameterType);

				// TODO
				switch (parameterType)
				{
					case ChunkParameter::ChunkParameterType::HEARTBEAT_INFO:
					{
						parameter = HeartbeatInfoChunkParameter::Parse(ptr, parameterLength + padding);

						break;
					}

					default:
					{
						parameter = UnknownChunkParameter::Parse(ptr, parameterLength + padding);
					}
				}

				if (!parameter)
				{
					return false;
				}

				this->parameters.push_back(parameter);

				ptr += parameter->GetLength();
			}

			const size_t computedLength = ptr - buffer;

			// Ensure computed length matches the total given buffer length.
			if (computedLength != bufferLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "computed length (%zu bytes) != buffer length (%" PRIu16 " bytes)",
				  computedLength,
				  bufferLength);

				return false;
			}

			return true;
		}
	} // namespace SCTP
} // namespace RTC
