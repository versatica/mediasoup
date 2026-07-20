#ifndef MS_RTC_SUBCHANNELS_CODEC_HPP
#define MS_RTC_SUBCHANNELS_CODEC_HPP

#include "common.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include <vector>

namespace RTC
{
	/**
	 * Helper to (optionally) encode/decode subchannels and required subchannel at
	 * the beginning of an SCTP message payload so that this information travels
	 * within the SCTP message itself.
	 */
	class SubchannelsCodec
	{
		/**
		 * Wire layout of the encoded header prepended to the message payload (all
		 * fields in network byte order). `R` is the `requiredSubchannelFlag`, i.e.
		 * the least significant bit of the Magic Token. The `requiredSubchannel`
		 * field is only present when `R` is 1.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       Magic Token (1/2)                       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      Magic Token (2/2)                      |R|
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        subchannelsCount       |          Subchannel 0         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              ...                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Subchannel N-1        |  requiredSubchannel (if R=1)  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

	public:
		/**
		 * 8-byte Magic Token that prefixes an encoded payload. Its least significant
		 * bit is reserved as the `requiredSubchannelFlag`, so it is always 0 in the
		 * token constant itself.
		 */
		static constexpr uint64_t MagicToken{ 0x5375624368616E00 };
		static constexpr uint64_t RequiredSubchannelFlagMask{ 0x1 };

	public:
		/**
		 * If `subchannels` is not empty or `requiredSubchannel` has value, encodes
		 * them at the beginning of the given message payload.
		 *
		 * @returns true if the message was encoded, false otherwise.
		 */
		static bool EncodeSubchannels(
		  RTC::SCTP::Message& message,
		  const std::vector<uint16_t>& subchannels,
		  std::optional<uint16_t> requiredSubchannel);

		/**
		 * If the given message payload starts with the Magic Token, extracts the
		 * encoded subchannels and (optionally) the required subchannel, fills the
		 * given arguments and removes the encoded header from the message payload.
		 *
		 * @returns true if the message was successfully decoded, false otherwise.
		 */
		static bool DecodeSubchannels(
		  RTC::SCTP::Message& message,
		  std::vector<uint16_t>& subchannels,
		  std::optional<uint16_t>& requiredSubchannel);
	};
} // namespace RTC

#endif
