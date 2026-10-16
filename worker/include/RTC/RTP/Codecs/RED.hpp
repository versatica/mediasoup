#ifndef MS_RTC_RTP_CODECS_RED_HPP
#define MS_RTC_RTP_CODECS_RED_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	namespace RTP
	{
		namespace Codecs
		{
			/**
			 * RTP Payload for Redundant Audio Data (RED).
			 *
			 * @see https://datatracker.ietf.org/doc/html/rfc2198
			 *
			 * Non final block header (4 bytes):
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |F|   block PT  |  timestamp offset         |   block length    |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 *
			 * Final block header (1 byte):
			 *
			 *  0 1 2 3 4 5 6 7
			 * +-+-+-+-+-+-+-+-+
			 * |0|   Block PT  |
			 * +-+-+-+-+-+-+-+-+
			 *
			 * Block headers are followed by the data of each block, being the data of
			 * the primary (last) block the one that takes the remaining payload.
			 */
			class RED
			{
			public:
				/**
				 * Parse the given RED payload and get the payload type of the primary
				 * block and the offset at which its data begins.
				 *
				 * @return `false` if the RED payload is invalid.
				 */
				static bool GetPrimaryBlock(
				  const uint8_t* data, size_t len, uint8_t& payloadType, size_t& offset);

				/**
				 * Rewrite the payload type of every block in the given RED payload by
				 * using the given mapping.
				 *
				 * @return `false` if the RED payload is invalid or if any block payload
				 *   type is not present in the mapping, in which case the payload is not
				 *   modified.
				 */
				static bool MapPayloadTypes(
				  uint8_t* data, size_t len, const ankerl::unordered_dense::map<uint8_t, uint8_t>& mapping);

				/**
				 * Convert the given RED packet into a regular RTP packet containing just
				 * the data of the primary block.
				 *
				 * @return `false` if the RED payload is invalid, in which case the packet
				 *   is not modified.
				 */
				static bool Unwrap(RTP::Packet* packet);
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
