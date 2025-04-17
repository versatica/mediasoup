#define MS_CLASS "RTC::SCTP::ChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/ChunkParameter.hpp"
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
		std::unordered_map<ChunkParameter::ChunkParameterType, std::string> ChunkParameter::chunkParameterType2String =
		{
			{ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,      "HEARTBEAT_INFO"      },
			{ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,        "IPV4_ADDRESS"        },
			{ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,        "IPV6_ADDRESS"        },
			{ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE, "COOKIE_PRESERVATIVE" },
			// TODO: Add more.
		};
		// clang-format on

		/* Class methods. */

		bool ChunkParameter::IsChunkParameter(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  ChunkParameterType& parameterType,
		  uint16_t& parameterLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (bufferLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_WARN_TAG(sctp, "no space for SCTP Chunk Parameter Header [bufferLength:%zu]", bufferLength);

				return false;
			}

			const auto* parameterHeader =
			  reinterpret_cast<const ChunkParameter::ChunkParameterHeader*>(buffer);

			parameterType = static_cast<ChunkParameterType>(
			  uint16_t{ ntohs(static_cast<uint16_t>(parameterHeader->type)) });
			parameterLength = uint16_t{ ntohs(parameterHeader->length) };

			if (parameterLength < ChunkParameter::ChunkParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "Chunk Parameter Length field must have value greater or equal than %zu",
				  ChunkParameter::ChunkParameterHeaderLength);

				return false;
			}

			// Parameter total length must be multiple of 4 bytes and must include
			// padding bytes despite Parameter Length field doesn't not include
			// padding.
			// NOTE: We must cast to size_t, otherwise a maximum Parameter Length
			// value of 65535 would generate a padded length of 0 bytes!
			size_t paddedParameterLength = Utils::Byte::PadTo4Bytes(size_t{ parameterLength });

			if (bufferLength < paddedParameterLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "no space for 4-byte padded announced Chunk Parameter Length [paddedParameterLength:%zu, bufferLength:%zu]",
				  paddedParameterLength,
				  bufferLength);

				return false;
			}

			padding = paddedParameterLength - parameterLength;

			return true;
		}

		const std::string& ChunkParameter::ChunkParameterType2String(ChunkParameterType parameterType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ChunkParameter::chunkParameterType2String.find(parameterType);

			if (it == ChunkParameter::chunkParameterType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		ChunkParameter::ChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		ChunkParameter::~ChunkParameter()
		{
			MS_TRACE();
		}

		void ChunkParameter::SoftCloneInto(ChunkParameter* parameter) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			parameter->SetLength(GetLength());
		}

		void ChunkParameter::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetType()),
			  ChunkParameter::ChunkParameterType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")",
			  GetLengthField(),
			  GetValueLength());
		}

		void ChunkParameter::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void ChunkParameter::InitializeHeader(ChunkParameterType parameterType, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetType(parameterType);
			SetLengthField(lengthFieldValue);
		}

		void ChunkParameter::SetLengthField(size_t length)
		{
			MS_TRACE();

			if (length > std::numeric_limits<uint16_t>::max())
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) cannot be greater than 65535", length);
			}

			GetHeaderPointer()->length = uint16_t{ htons(length) };
		}

		void ChunkParameter::SetValue(const uint8_t* value, uint16_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength      = GetLength();
			auto previousLengthField = GetLengthField();
			auto previousValueLength = GetValueLength();
			auto newNotPaddedLength =
			  size_t{ previousLengthField } - size_t{ previousValueLength } + size_t{ valueLength };
			auto newPaddedLength = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			try
			{
				// Let's call SetLength() on parent with the new computed Parameter
				// length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				// NOTE: Chunk Parameters must be padded to 4 bytes.
				SetLength(newPaddedLength);

				// Update the Chunk Parameter Length field.
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

			// Copy the given value into the buffer.
			std::memmove(GetValuePointer(), value, valueLength);

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}
	} // namespace SCTP
} // namespace RTC
