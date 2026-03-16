#ifndef MS_RTC_SCTP_NEGOTIATED_CAPABILITIES_HPP
#define MS_RTC_SCTP_NEGOTIATED_CAPABILITIES_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/AnyInitChunk.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Those are the SCTP association capabilities negotiated during the
		 * handshake, meaning that both endpoints support them and have agreed on
		 * them.
		 */
		struct NegotiatedCapabilities
		{
			/**
			 * Create a NegotiatedCapabilities struct. Intended to be used during
			 * the SCTP association handshake flow.
			 *
			 * @remarks
			 * - Given `remoteChunk` must be an INIT or an INIT_ACK Chunk.
			 */
			static NegotiatedCapabilities Factory(
			  const SctpOptions& sctpOptions, const AnyInitChunk* remoteChunk);

			/**
			 * Negotiated maximum number of outbound streams (OS).
			 */
			uint16_t maxOutboundStreams{ 0 };

			/**
			 * Negotiated maximum number of inbound streams (MIS).
			 */
			uint16_t maxInboundStreams{ 0 };

			/**
			 * Partial Reliability Extension.
			 *
			 * @see RFC 3758.
			 */
			bool partialReliability{ false };

			/**
			 * Stream Schedulers and User Message Interleaving (I-DATA Chunks).
			 *
			 * @see RFC 8260.
			 */
			bool messageInterleaving{ false };

			/**
			 * Stream Re-Configuration.
			 *
			 * @see RFC 6525.
			 */
			bool reConfig{ false };

			/**
			 * Zero Checksum.
			 *
			 * @see RFC 9653.
			 */
			bool zeroChecksum{ false };

			void Dump(int indentation = 0) const;
		};
	} // namespace SCTP
} // namespace RTC

#endif
