#ifndef MS_RTC_SCTP_ASSOCIATION_METRICS_HPP
#define MS_RTC_SCTP_ASSOCIATION_METRICS_HPP

#include "common.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Association metrics.
		 */
		struct AssociationMetrics
		{
			/**
			 * Number of SCTP Packets sent.
			 */
			uint64_t txPacketsCount{ 0 };

			/**
			 * Number of messages requested to be sent.
			 */
			uint64_t txMessagesCount{ 0 };

			/**
			 * Number of SCTP Packets received.
			 */
			uint64_t rxPacketsCount{ 0 };

			/**
			 * Number of messages received.
			 */
			uint64_t rxMessagesCount{ 0 };

			/**
			 * Number of Packets retransmitted. Since SCTP Packets can contain both
			 * retransmitted DATA or I-DATA Chunks and Chunks that are transmitted for
			 * the first time, this represents an upper bound as it's incremented
			 * every time a Packet contains a retransmitted DATA or I-DATA chunk.
			 */
			uint64_t rtxPacketsCount{ 0 };

			/**
			 * Total number of bytes retransmitted. This includes the payload and
			 * DATA/I-DATA headers, but not SCTP packet headers.
			 */
			uint64_t rtxBytesCount{ 0 };

			/**
			 * The current congestion window (cwnd) in bytes, corresponding to
			 * `spinfo_cwnd` defined in RFC 6458.
			 */
			size_t cwndBytes{ 0 };

			/**
			 * Smoothed round trip time (in ms), corresponding to `spinfo_srtt`
			 * defined in RFC 6458.
			 */
			uint64_t srttMs{ 0 };

			/**
			 * Number of data items in the retransmission queue that haven’t been
			 * acked/nacked yet and are in-flight. Corresponding to `sstat_unackdata`
			 * defined in RFC 6458. This may be an approximation when there are
			 * messages in the send queue that haven't been fragmented/packetized yet.
			 */
			size_t unackDataCount{ 0 };

			/**
			 * The peer’s last announced receiver window size, corresponding to
			 * `sstat_rwnd` defined in RFC 6458.
			 */
			uint32_t peerRwndBytes{ 0 };

			/**
			 * SCTP implementation of the peer. Only detected when the peer sends an
			 * INIT_ACK Chunk to us with a State Cookie.
			 */
			Types::SctpImplementation peerImplementation{ Types::SctpImplementation::UNKNOWN };

			/**
			 * The number of negotiated outbound streams, which is configured locally
			 * as `SctpOptions::maxOutboundStreams`, and which will be signaled by the
			 * remote during connection.
			 */
			uint16_t negotiatedMaxOutboundStreams{ 0 };

			/**
			 * The number of negotiated inbound streams, which is configured locally
			 * as `SctpOptions::maxInboundStreams`, and which will be signaled by the
			 * remote during connection.
			 */
			uint16_t negotiatedMaxInboundStreams{ 0 };

			/**
			 * Whether Partial Reliability has been negotiated.
			 *
			 * @see RFC 3758.
			 */
			bool usesPartialReliability{ false };

			/**
			 * Whether Stream Schedulers and User Message Interleaving (I-DATA Chunks)
			 * have been negotiated.
			 *
			 * @see RFC 8260.
			 */
			bool usesMessageInterleaving{ false };

			/**
			 * Whether Stream Re-Configuration has been negotiated.
			 *
			 * @see RFC 6525.
			 */
			bool usesReConfig{ false };

			/**
			 * Whether Alternate Error Detection Method for Zero Checksum has been
			 * negotiated.
			 *
			 * @remarks
			 * - This feature is only enabled if both peers signal their wish to use
			 *   the same (non-zero) Zero Checksum Alternate Error Detection Method.
			 *
			 * @see RFC 9653.
			 */
			bool usesZeroChecksum{ false };

			void Dump(int indentation = 0) const;
		};
	} // namespace SCTP
} // namespace RTC

#endif
