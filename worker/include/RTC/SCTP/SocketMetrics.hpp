#ifndef MS_RTC_SCTP_SOCKET_METRICS_HPP
#define MS_RTC_SCTP_SOCKET_METRICS_HPP

#include "common.hpp"
#include "RTC/SCTP/StateCookie.hpp"
#include "RTC/SCTP/Types.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Socket metrics.
		 */
		struct SocketMetrics
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
			 * Number of Packets retransmitted. Since SCTP Packets can contain both
			 * retransmitted DATA or I-DATA Chunks and Chunks that are transmitted for
			 * the first time, this represents an upper bound as it's incremented
			 * every time a Packet contains a retransmitted DATA or I-DATA chunk.
			 */
			uint64_t rtxPacketsCount{ 0 };

			// TODO: More.

			/**
			 * Number of SCTP Packets received.
			 */
			uint64_t rxPacketsCount{ 0 };
			/**
			 * Number of messages received.
			 */
			uint64_t rxMessagesCount{ 0 };

			// TODO: More.

			/**
			 * SCTP implementation of the peer. Only detected when the peer sends an
			 * INIT_ACK Chunk to us with a State Cookie.
			 */
			Types::SctpImplementation peerImplementation{ Types::SctpImplementation::UNKNOWN };

			// TODO: More.

			/**
			 * Whether Stream Schedulers and User Message Interleaving (I-DATA Chunks)
			 * have been negotiated.
			 *
			 * @see RFC 8260.
			 */
			bool messageInterleaving{ false };
			/**
			 * Whether Alternate Error Detection Method for Zero Checksum has been
			 * negotiated.
			 *
			 * @remarks
			 * This feature is only enabled if both peers signal their wish to use
			 * the same (non-zero) Zero Checksum Alternate Error Detection Method.
			 *
			 * @see RFC 9653.
			 */
			bool zeroChecksum{ false };

			void Dump(int indentation = 0) const;
		};
	} // namespace SCTP
} // namespace RTC

#endif
