#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Chunk.hpp"
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
		  size_t& chunkLength,
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
			MS_DUMP("</Chunk>");
		}

		Chunk* Chunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new Chunk(buffer, bufferLength);

			CloneInto(clonedChunk);

			return clonedChunk;
		}

		void Chunk::InitializeHeader(ChunkType chunkType, uint8_t flags, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			GetHeaderPointer()->type  = chunkType;
			GetHeaderPointer()->flags = flags;
			SetLengthField(lengthFieldValue);
		}
	} // namespace SCTP
} // namespace RTC
