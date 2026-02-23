#ifndef MS_RTC_SCTP_SOCKET_OPTIONS_HPP
#define MS_RTC_SCTP_SOCKET_OPTIONS_HPP

#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Options given to Socket constructor.
		 */
		struct SocketOptions
		{
			/**
			 * Signaled source port.
			 */
			uint16_t sourcePort{ 0 };
			/**
			 * Signaled destination port.
			 */
			uint16_t destinationPort{ 0 };
			/**
			 * Announced maximum number of outbound streams (OS).
			 * NOTE: We use maximum value by default.
			 */
			uint16_t maxOutboundStreams{ 65535 };
			/**
			 * Announced maximum number of inbound streams (MIS).
			 * NOTE: We use maximum value by default.
			 */
			uint16_t maxInboundStreams{ 65535 };
			/**
			 * Maximum received window buffer size. It must be larger than the
			 * largest sized message we want to be able to receive.
			 *
			 * @remarks
			 * Default value copied from dcSCTP library.
			 */
			uint32_t localAdvertisedReceiverWindowCredit{ 5 * 1024 * 1024 };
			/**
			 * Use Partial Reliability Extension.
			 * @see RFC 3758.
			 */
			bool partialReliability{ false };
			/**
			 * Use Stream Schedulers and User Message Interleaving (I-DATA Chunks).
			 *
			 * @see RFC 8260.
			 */
			bool messageInterleaving{ false };
			/**
			 * Alternate Error Detection Method for Zero Checksum.
			 *
			 * @remarks
			 * This feature is only enabled if both peers signal their wish to use
			 * the same (non-zero) Zero Checksum Alternate Error Detection Method.
			 *
			 * @see RFC 9653.
			 */
			ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod zeroChecksumAlternateErrorDetectionMethod{
				ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
			};
			/**
			 * Maximum size of a SCTP Packet. It doesn't include any overhead of
			 * DTLS, TURN, UDP or IP headers.
			 */
			size_t mtu{ RTC::Consts::MaxSafeMtuSizeForSctp };
			/**
			 * T1-init timeout (ms).
			 */
			uint64_t t1InitTimeout{ 1000 };
			/**
			 * T1-cookie timeout (ms).
			 */
			uint64_t t1CookieTimeout{ 1000 };
			/**
			 * T2-shutdown timeout (ms).
			 */
			uint64_t t2ShutdownTimeout{ 1000 };
			/**
			 * Maximum duration of the backoff timeout. If no value is given, no
			 * limit is set.
			 */
			std::optional<uint64_t> timerMaxBackoffTimeout{ std::nullopt };
			/**
			 * Max.Init.Retransmits. Set to std::nullopt for no limit.
			 *
			 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-16
			 */
			std::optional<size_t> maxInitRetransmits = 8;
			/**
			 * Maximum data retransmit attempts (for DATA, I_DATA and other Chunks).
			 * Set to std::nullopt for no limit.
			 */
			std::optional<size_t> maxRetransmits = 8;
		};
	} // namespace SCTP
} // namespace RTC

#endif
