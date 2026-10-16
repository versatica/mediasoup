#ifndef MS_RTC_RTP_CODECS_RED_HPP
#define MS_RTC_RTP_CODECS_RED_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <ankerl/unordered_dense.h>
#include <vector>

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
				 * Data removed from a RED packet by Unwrap() so the packet can be
				 * restored later by Restore().
				 */
				struct RemovedData
				{
					uint8_t payloadType{ 0 };
					uint8_t paddingLength{ 0 };
					// RED block headers and redundant blocks data.
					std::vector<uint8_t> data;
				};

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
				 * Convert the given RED packet (in place) into a regular RTP packet
				 * containing just the data of the primary block. If `removedData` is
				 * given, the removed data is stored into it so the packet can be
				 * restored later by Restore().
				 *
				 * @return `false` if the RED payload is invalid, in which case the packet
				 *   is not modified.
				 */
				static bool Unwrap(RTP::Packet* packet, RemovedData* removedData = nullptr);

				/**
				 * Restore a RED packet previously converted by Unwrap().
				 */
				static void Restore(RTP::Packet* packet, const RemovedData& removedData);

				/**
				 * Process the given RED packet at codec level by inspecting its primary
				 * block, whose codec is given (i.e. Opus DTX detection).
				 */
				static void ProcessRtpPacket(RTP::Packet* packet, const RTC::RtpCodecMimeType& mimeType);
			};
		} // namespace Codecs
	} // namespace RTP
} // namespace RTC

#endif
