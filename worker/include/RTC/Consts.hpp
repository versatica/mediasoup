#ifndef MS_RTC_CONSTS_HPP
#define MS_RTC_CONSTS_HPP

namespace RTC
{
	namespace Consts
	{
		/**
		 * Max MTU size.
		 */
		constexpr size_t MtuSize{ 1500u };

		/**
		 * Maximum size for a RTCP compound packet.
		 * IPv4|Ipv6 header size: 20|40 bytes. IPv6 considered.
		 * UDP|TCP header size:   8|20  bytes. TCP considered.
		 * SRTP Encryption: 148 bytes.
		     SRTP_MAX_TRAILER_LEN+4 is the maximum number of octects that will be
		     added to an RTCP packet by srtp_protect_rtcp().
		     srtp.h: SRTP_MAX_TRAILER_LEN (SRTP_MAX_TAG_LEN + SRTP_MAX_MKI_LEN).
		 */
		constexpr size_t RtcpPacketMaxSize{ RTC::Consts::MtuSize - 40 - 20 - 148u };

		/**
		 * MID RTP header extension max length (just used when setting/updating MID
		 * extension).
		 */
		constexpr uint8_t MidRtpExtensionMaxLength{ 8u };
	} // namespace Consts
} // namespace RTC

#endif
