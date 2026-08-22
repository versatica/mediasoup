#ifndef MS_RTC_SCTP_CAPABILITIES_HPP
#define MS_RTC_SCTP_CAPABILITIES_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/AnyInitChunk.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Those are the raw SCTP capabilities announced by an endpoint in its INIT
		 * or INIT-ACK chunk, before any negotiation against local options takes
		 * place.
		 */
		struct Capabilities
		{
			/**
			 * Create a Capabilities struct holding the capabilities announced by
			 * the remote endpoint. No local option is applied here.
			 *
			 * @remarks
			 * - Given `remoteChunk` must be an INIT or an INIT-ACK chunk.
			 */
			static Capabilities Factory(const AnyInitChunk* remoteChunk);

			/**
			 * Maximum number of outbound streams (OS).
			 */
			uint16_t maxOutboundStreams{ 0 };

			/**
			 * Maximum number of inbound streams (MIS).
			 */
			uint16_t maxInboundStreams{ 0 };

			/**
			 * Partial Reliability Extension.
			 *
			 * @see RFC 3758.
			 */
			bool partialReliability{ false };

			/**
			 * Stream Schedulers and User Message Interleaving (I-DATA chunks).
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
			 * Alternate Error Detection Method for Zero Checksum.
			 *
			 * @see RFC 9653.
			 */
			ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod zeroChecksumAlternateErrorDetectionMethod{
				ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
			};

			void Dump(int indentation = 0) const;
		};
	} // namespace SCTP
} // namespace RTC

#endif
