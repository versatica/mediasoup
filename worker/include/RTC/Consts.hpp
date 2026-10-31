#ifndef MS_RTC_CONSTS_HPP
#define MS_RTC_CONSTS_HPP

#include "common.hpp"
#include <string_view>

namespace RTC
{
	namespace Consts
	{
		/**
		 * Bytes an IPv4 header takes, without options.
		 */
		constexpr size_t Ipv4HeaderSize{ 20 };

		/**
		 * Bytes an IPv6 header takes, without extension headers.
		 */
		constexpr size_t Ipv6HeaderSize{ 40 };

		/**
		 * Bytes a UDP header takes.
		 */
		constexpr size_t UdpHeaderSize{ 8 };

		/**
		 * Bytes a TCP header takes, without options.
		 */
		constexpr size_t TcpHeaderSize{ 20 };

		/**
		 * Bytes of the length field that frames every packet sent over TCP.
		 *
		 * @see https://datatracker.ietf.org/doc/html/rfc4571
		 */
		constexpr size_t TcpFramingSize{ 2 };

		/**
		 * Largest RTP packet accepted from the network, measured as received (so
		 * encrypted if SRTP is in use). Larger ones are discarded.
		 */
		constexpr size_t IncomingRtpPacketMaxSize{ 2048 };

		/**
		 * Largest RTP packet sent, measured as sent (so encrypted if SRTP is in
		 * use). RTP packets are forwarded rather than generated, and they grow on
		 * the way (header extensions, RTX encoding, SRTP), so received ones that
		 * could exceed this size once sent are discarded on arrival. It matches
		 * `IncomingRtpPacketMaxSize`, so whatever is sent is accepted by the
		 * receiver.
		 */
		constexpr size_t OutgoingRtpPacketMaxSize{ 2048 };

		/**
		 * Bytes that RTX encoding adds to a RTP packet (the original sequence
		 * number).
		 *
		 * @see https://datatracker.ietf.org/doc/html/rfc4588#section-4
		 */
		constexpr size_t RtxOverhead{ 2 };

		/**
		 * Most bytes that SRTP adds to a RTP packet when protecting it (the longest
		 * authentication tag).
		 */
		constexpr size_t SrtpMaxOverhead{ 16 };

		/**
		 * Largest RTCP packet accepted from the network, measured as received (so
		 * encrypted if SRTP is in use). Larger ones are discarded.
		 */
		constexpr size_t IncomingRtcpPacketMaxSize{ 2048 };

		/**
		 * Largest RTCP packet generated (compound packets, transport-cc feedback,
		 * NACK...), measured before SRTP is applied. Content that does not fit in a
		 * packet is split into several.
		 */
		constexpr size_t OutgoingRtcpPacketMaxSize{ 1200 };

		/**
		 * Largest DTLS datagram generated during the handshake.
		 */
		constexpr size_t OutgoingDtlsPacketMaxSize{ 1200 };

		/**
		 * Max length for a 1 byte RTP header extension.
		 */
		constexpr uint8_t OneByteRtpExtensionMaxLength{ 16 };

		/**
		 * Max length for a 2 bytes RTP header extension.
		 */
		constexpr uint8_t TwoBytesRtpExtensionMaxLength{ 255 };

		/**
		 * MID RTP header extension max length (just used when setting/updating MID
		 * extension).
		 */
		constexpr uint8_t MidRtpExtensionMaxLength{ 8 };

		/**
		 * SSRC of the RTP stream the probing packets are sent on.
		 *
		 * @remarks
		 * - The receiver negotiates a stream with this very SSRC, so it cannot be
		 *   changed without changing the client as well.
		 */
		constexpr uint32_t BweProbeRtpSsrc{ 1234 };

		/**
		 * Codec payload type of the RTP stream the probing packets are sent on.
		 *
		 * @remarks
		 * - The receiver negotiates a stream with this very payload type, so it
		 *   cannot be changed without changing the client as well.
		 */
		constexpr uint8_t BweProbeRtpPayloadType{ 127 };

		/**
		 * MID of the RTP stream the probing packets are sent on, which is how the
		 * receiver tells it apart.
		 *
		 * @remarks
		 * - The receiver negotiates a stream with this very MID, so it cannot be
		 *   changed without changing the client as well.
		 */
		constexpr std::string_view BweProbeRtpMid{ "probator" };

		static_assert(
		  BweProbeRtpMid.size() <= MidRtpExtensionMaxLength,
		  "the MID of the probing stream does not fit in a MID RTP header extension");

		/**
		 * Lowest bitrate the bandwidth estimation ever produces (bps), so 5 kbps.
		 */
		constexpr int64_t BweMinBitrate{ 5000 };

		/**
		 * Highest bitrate the bandwidth estimation ever deals in (bps), so 1 Gbps.
		 */
		constexpr int64_t BweMaxBitrate{ 1000000000 };

		/**
		 * Largest safe SCTP packet. Starting from the minimum guaranteed MTU value
		 * of 1280 for IPv6 (which may not support fragmentation), take off 85
		 * bytes for DTLS/TURN/TCP/IP and ciphertext overhead.
		 *
		 * Additionally, it's possible that TURN adds an additional 4 bytes of
		 * overhead after a channel has been established, so an additional 4 bytes
		 * is subtracted.
		 *
		 * 1280 IPV6 MTU
		 *  -40 IPV6 header
		 *   -8 UDP
		 *  -24 GCM Cipher
		 *  -13 DTLS record header
		 *   -4 TURN ChannelData
		 * = 1191 bytes.
		 *
		 * @remarks
		 * Value copied from dcSCTP library.
		 */
		constexpr size_t OutgoingSctpPacketMaxSize{ 1191 };

		/**
		 * Smallest SCTP receiver window buffer size allowed. A DATA chunk is only
		 * checked against the window before being added to the reassembly queue,
		 * so the window must be larger than any DATA chunk payload, which is
		 * bounded by the largest packet that can be received.
		 */
		constexpr size_t SctpReceiverWindowBufferMinSize{ 65536 };
	} // namespace Consts
} // namespace RTC

#endif
