#define MS_CLASS "RTC::SCTP::Chunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Chunk.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <bitset> // std::bitset()

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		absl::flat_hash_map<Chunk::ChunkType, std::string> Chunk::chunkType2String =
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

		Chunk* Chunk::Parse(const uint8_t* data, size_t len, bool exactLen)
		{
			MS_TRACE();

			// Ensure there are at least 4 bytes (chunk value is zero-length).
			if (len < HeaderSize)
			{
				MS_WARN_TAG(sctp, "not an SCTP chunk");

				return nullptr;
			}

			auto* chunk = new Chunk(data, len);

			// Pointer that initially points to the given data buffer and is later
			// incremented to point to other parts of the chunk.
			auto* ptr = const_cast<uint8_t*>(data);

			// Inspect data after the header size, so move to the chunk value.
			ptr += HeaderSize;

			auto valueLengthWithPadding = Utils::Byte::PadTo4Bytes(chunk->GetValueLength());

			// Ensure there is space for the chunk value and its possible padding.
			if (len - (ptr - data) < valueLengthWithPadding)
			{
				MS_WARN_TAG(sctp, "the chunk length exceeds the remaining size, chunk discarded");

				delete chunk;
				return nullptr;
			}

			// switch (chunk->GetType())
			// {
			//   TODO
			// }

			// Move pointer after chunk value and its padding.
			ptr += valueLengthWithPadding;

			// If `exactLen` is set, ensure current position matches the total length.
			if (exactLen)
			{
				//
				// TODO: As per RFC 9260:
				//
				// Note: A robust implementation is expected to accept the chunk whether
				// or not the final padding has been included in the Chunk Length.
				if (ptr - data != len)
				{
					MS_WARN_TAG(sctp, "computed chunk size does not match total size, chunk discarded");

					delete chunk;
					return nullptr;
				}
			}
			// Otherwise we have to fix chunk total size.
			else
			{
				chunk->SetSize(ptr - data);
			}

			return chunk;
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

		Chunk::Chunk(const uint8_t* data, size_t size)
		  : data(const_cast<uint8_t*>(data)), size(size),
		    header(reinterpret_cast<Header*>(const_cast<uint8_t*>(data)))
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

			MS_DUMP("  size: %zu", GetSize());

			MS_DUMP("  type: %" PRIu8 " (%s)", GetType(), Chunk::ChunkType2String(GetType()).c_str());

			std::bitset<8> flagsBitset(this->GetFlags());

			MS_DUMP("  flags: %s", flagsBitset.to_string().c_str());

			MS_DUMP("  value length: %" PRIu16, GetValueLength());

			MS_DUMP("</Chunk>");
		}
	} // namespace SCTP
} // namespace RTC
