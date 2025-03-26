#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Packet.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		absl::flat_hash_map<Packet::ChunkType, std::string> Packet::chunkType2String =
		{
			{ Packet::ChunkType::DATA,              "DATA"              },
			{ Packet::ChunkType::INIT,              "INIT"              },
			{ Packet::ChunkType::INIT_ACK,          "INIT_ACK"          },
			{ Packet::ChunkType::SACK,              "SACK"              },
			{ Packet::ChunkType::HEARTBEAT,         "HEARTBEAT"         },
			{ Packet::ChunkType::HEARTBEAT_ACK,     "HEARTBEAT_ACK"     },
			{ Packet::ChunkType::ABORT,             "ABORT"             },
			{ Packet::ChunkType::SHUTDOWN,          "SHUTDOWN"          },
			{ Packet::ChunkType::SHUTDOWN_ACK,      "SHUTDOWN_ACK"      },
			{ Packet::ChunkType::ERROR,             "ERROR"             },
			{ Packet::ChunkType::COOKIE_ECHO,       "COOKIE_ECHO"       },
			{ Packet::ChunkType::COOKIE_ACK,        "COOKIE_ACK"        },
			{ Packet::ChunkType::ECNE,              "ECNE"              },
			{ Packet::ChunkType::CWR,               "CWR"               },
			{ Packet::ChunkType::SHUTDOWN_COMPLETE, "SHUTDOWN_COMPLETE" }
		};
		// clang-format on

		/* Class methods. */

		Packet* Packet::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			if (!Packet::IsSctp(data, len))
			{
				return nullptr;
			}

			auto* ptr = const_cast<uint8_t*>(data);

			// Get the common header.
			auto* header = reinterpret_cast<CommonHeader*>(ptr);

			auto* packet = new Packet(header, len);

			// TODO: Remove.
			packet->Dump();

			if (packet->GetSourcePort() == 0u || packet->GetDestinationPort() == 0u)
			{
				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, packet discarded");

				delete packet;
				return nullptr;
			}

			// Start looking for chunks after SCTP common header.
			// Inspect data after the minimum header size.
			ptr += CommonHeaderSize;

			// Ensure there are at least 4 remaining bytes (chuck with 0 length).
			while (len - (ptr - data) >= 4)
			{
				// Get the chunk type.
				auto chunkType = static_cast<ChunkType>(Utils::Byte::Get1Byte(ptr, 0));

				// Get the chunk flags.
				const uint8_t chunkFlags = Utils::Byte::Get1Byte(ptr, 1);

				// Get the chunk length.
				const uint16_t chunkLength = Utils::Byte::Get2Bytes(ptr, 2);

				MS_DUMP(
				  "  [chunkType:%" PRIu8 " (%s), chunkFlags:%" PRIu8 ", chunkLength:%" PRIu16 "]",
				  chunkType,
				  Packet::ChunkType2String(chunkType).c_str(),
				  chunkFlags,
				  chunkLength);

				// Chunk Length includes the whole chunk (type, flags, length and value)
				// so minimum value is 4.
				if (chunkLength < 4u)
				{
					MS_WARN_TAG(sctp, "chunk length must be >= 4");

					delete packet;
					return nullptr;
				}

				// Ensure the chunk length is not greater than the remaining size.
				// NOTE: Take into account that Chunk Length includes the whole chunk.
				if ((ptr - data) + chunkLength > len)
				{
					MS_WARN_TAG(sctp, "the chunk length exceeds the remaining size, packet discarded");

					delete packet;
					return nullptr;
				}

				// switch (chunkType)
				// {
				//   TODO
				// }

				// Set next chunk position.
				ptr += static_cast<size_t>(Utils::Byte::PadTo4Bytes(static_cast<uint16_t>(chunkLength)));
			}

			// Ensure current position matches the total length.
			//
			// TODO: As per RFC 9260:
			//
			// Note: A robust implementation is expected to accept the chunk whether
			// or not the final padding has been included in the Chunk Length.
			if (ptr - data != len)
			{
				MS_WARN_TAG(sctp, "computed packet size does not match total size, packet discarded");

				delete packet;
				return nullptr;
			}

			return packet;
		}

		const std::string& Packet::ChunkType2String(ChunkType chunkType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Packet::chunkType2String.find(chunkType);

			if (it == Packet::chunkType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		Packet::Packet(CommonHeader* commonHeader, size_t size) : commonHeader(commonHeader), size(size)
		{
			MS_TRACE();
		}

		Packet::~Packet()
		{
			MS_TRACE();
		}

		void Packet::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<Packet>");
			MS_DUMP("  size: %zu", GetSize());
			MS_DUMP("  source port: %" PRIu16, GetSourcePort());
			MS_DUMP("  destination port: %" PRIu16, GetDestinationPort());
			MS_DUMP("  verification tag: %" PRIu32, GetVerificationTag());
			MS_DUMP("  checksum: %" PRIu32, GetChecksum());
			MS_DUMP("</Packet>");
		}
	} // namespace SCTP
} // namespace RTC
