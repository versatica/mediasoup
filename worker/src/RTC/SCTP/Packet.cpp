#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/Packet.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		Packet* Packet::Parse(const uint8_t* data, size_t len)
		{
			MS_TRACE();

			if (!Packet::IsSctp(data, len))
			{
				return nullptr;
			}

			MS_DUMP("<BEGIN>");

			MS_DUMP("  [len:%zu]", len);

			/**
			 * SCTP Common Header.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |      Source Port Number       |    Destination Port Number    |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                       Verification Tag                        |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                           Checksum                            |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

			const uint16_t sourcePort      = Utils::Byte::Get2Bytes(data, 0);
			const uint16_t destinationPort = Utils::Byte::Get2Bytes(data, 2);

			MS_DUMP("  [sourcePort:%" PRIu16 ", destinationPort:%" PRIu16 "]", sourcePort, destinationPort);

			if (sourcePort == 0u || destinationPort == 0u)
			{
				MS_WARN_TAG(sctp, "source port and destination port cannot be 0, packet discarded");

				return nullptr;
			}

			const uint32_t verificationTag = Utils::Byte::Get4Bytes(data, 4);
			const uint32_t checksum        = Utils::Byte::Get4Bytes(data, 8);

			MS_DUMP("  [verificationTag:%" PRIu32 ", checksum:%" PRIu32 "]", verificationTag, checksum);

			auto* packet = new Packet();

			// Start looking for attributes after SCTP common header (Byte #12).
			size_t pos{ 12 };

			/**
			 * SCTP Chunk.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |  Chunk Type   |  Chunk Flags  |         Chunk Length          |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * \                                                               \
			 * /                          Chunk Value                          /
			 * \                                                               \
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

			// Ensure there are at least 4 remaining bytes (chuck with 0 length).
			while (pos + 4 <= len)
			{
				// Get the chunk type.
				auto chunkType = static_cast<ChunkType>(Utils::Byte::Get1Byte(data, pos));

				// Get the chunk flags.
				const uint8_t chunkFlags = Utils::Byte::Get1Byte(data, pos + 1);

				// Get the chunk length.
				const uint16_t chunkLength = Utils::Byte::Get2Bytes(data, pos + 2);

				MS_DUMP(
				  "  [chunkType:%" PRIu8 ", chunkFlags:%" PRIu8 ", chunkLength:%" PRIu16 "]",
				  chunkType,
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
				if ((pos + chunkLength) > len)
				{
					MS_WARN_TAG(sctp, "the chunk length exceeds the remaining size, packet discarded");

					delete packet;
					return nullptr;
				}

				const uint8_t* chunkValuePos = data + pos + 4;

				// switch (chunkType)
				// {
				//   TODO
				// }

				// Set next chunk position.
				pos = static_cast<size_t>(Utils::Byte::PadTo4Bytes(static_cast<uint16_t>(pos + chunkLength)));
			}

			// Ensure current position matches the total length.
			//
			// TODO: As per RFC 9260:
			//
			// Note: A robust implementation is expected to accept the chunk whether
			// or not the final padding has been included in the Chunk Length.
			if (pos != len)
			{
				MS_WARN_TAG(sctp, "computed packet size does not match total size, packet discarded");

				delete packet;
				return nullptr;
			}

			MS_DUMP("<END>");

			return packet;
		}
	} // namespace SCTP
} // namespace RTC
