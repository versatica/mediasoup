#define MS_CLASS "RTC::SCTP::ChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/ChunkParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ChunkParameter::ChunkParameterType, std::string> ChunkParameter::chunkParameterType2String =
		{
			{ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO, "HEARTBEAT_INFO" },
			// TODO
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

		ChunkParameter::ChunkParameter(const uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			// NOTE: No need to this in each subclass since header of Chunk
			// Parameters has fixed length.
			SetLength(ChunkParameter::ChunkParameterHeaderLength);
		}

		ChunkParameter::~ChunkParameter()
		{
			MS_TRACE();
		}

		void ChunkParameter::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<ChunkParameter>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu16 " (%s) (unknown:%s)",
			  static_cast<uint16_t>(GetType()),
			  ChunkParameter::ChunkParameterType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP(
			  "  length field: %" PRIu16 " (value length: %" PRIu16 ")", GetLengthField(), GetValueLength());
			MS_DUMP("</ChunkParameter>");
		}

		ChunkParameter* ChunkParameter::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new ChunkParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void ChunkParameter::InitializeHeader(ChunkParameterType parameterType)
		{
			MS_TRACE();

			SetType(parameterType);
			SetLengthField(ChunkParameter::ChunkParameterHeaderLength);
		}

		void ChunkParameter::SetLengthField(size_t length)
		{
			MS_TRACE();

			if (length > 65535u)
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) cannot be greater than 65535", length);
			}

			GetHeaderPointer()->length = uint16_t{ htons(length) };
		}
	} // namespace SCTP
} // namespace RTC
