#ifndef MS_RTC_SCTP_NEGOTIATED_CAPABILITIES_HPP
#define MS_RTC_SCTP_NEGOTIATED_CAPABILITIES_HPP

#include "common.hpp"

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
			 * Negotiated maximum number of outbound streams (OS).
			 */
			uint16_t maxOutboundStreams{ 0 };
			/**
			 * Negotiated maximum number of inbound streams (MIS).
			 */
			uint16_t maxInboundStreams{ 0 };
			/**
			 * Partial Reliability Extension.
			 * @see RFC 3758.
			 */
			bool partialReliability{ false };
			/**
			 * Stream Schedulers and User Message Interleaving (I-DATA Chunks).
			 * @see RFC 8260.
			 */
			bool messageInterleaving{ false };
			/**
			 * Stream Reconfiguration.
			 * @see RFC 6525.
			 */
			bool reconfig{ false };
			/**
			 * Zero Checksum.
			 * @see RFC 9653.
			 */
			bool zeroChecksum{ false };

			void Dump(int indentation = 0) const;
		};
	} // namespace SCTP
} // namespace RTC

#endif
