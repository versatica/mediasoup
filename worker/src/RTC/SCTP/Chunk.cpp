#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Chunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

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
		  const uint8_t* buffer, size_t bufferLength, ChunkType& chunkType, uint16_t& chunkLength)
		{
			MS_TRACE();

			if (bufferLength < Chunk::ChunkHeaderLength)
			{
				MS_WARN_TAG(sctp, "no space for SCTP Chunk header [bufferLength:%zu]", bufferLength);

				return false;
			}

			const auto* chunkHeader = reinterpret_cast<const Chunk::ChunkHeader*>(buffer);
			// Chunk total length must be multiple of 4 bytes and must include
			// padding bytes despite Chunk Length field doesn't not include padding.
			auto paddedChunkLength = Utils::Byte::PadTo4Bytes(uint16_t{ ntohs(chunkHeader->length) });

			if (bufferLength < paddedChunkLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "no space for padded announced Chunk Length field [paddedChunkLength:%" PRIu16 "]",
				  paddedChunkLength);

				return false;
			}

			chunkType   = chunkHeader->type;
			chunkLength = paddedChunkLength;

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
		}

		void Chunk::Dump() const
		{
			MS_TRACE();
			MS_DUMP("<Chunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  GetType(),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("  flags: " MS_UINT8_4BITS_TO_BINARY_PATTERN, MS_UINT8_4BITS_TO_BINARY(GetFlags()));
			MS_DUMP(
			  "  length field: %" PRIu16 " (computed chunk length: %" PRIu16 ")",
			  GetLengthField(),
			  GetValueLength());
			MS_DUMP("</Chunk>");
		}

		Chunk* Chunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			if (bufferLength < GetLength())
			{
				MS_THROW_TYPE_ERROR(
				  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
				  bufferLength,
				  GetLength());
			}

			std::memcpy(buffer, GetBuffer(), GetLength());

			auto* clonedChunk = new Chunk(buffer, bufferLength);

			// NOTE: The `frozen` flag will be false in the cloned Chunk by default.

			// Need to manually set Serializable length.
			clonedChunk->SetLength(GetLength());

			return clonedChunk;
		}

		void Chunk::InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t valueLength)
		{
			MS_TRACE();

			GetHeaderPointer()->type  = chunkType;
			GetHeaderPointer()->flags = flags;
			SetLengthField(Chunk::ChunkHeaderLength + valueLength);
		}
	} // namespace SCTP
} // namespace RTC
