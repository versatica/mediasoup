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
			 *
			 * @remarks
			 * - We use maximum value by default.
			 */
			uint16_t maxOutboundStreams{ 65535 };
			/**
			 * Announced maximum number of inbound streams (MIS).
			 *
			 * @remarks
			 * - We use maximum value by default.
			 */
			uint16_t maxInboundStreams{ 65535 };
			/**
			 * Maximum size of a SCTP Packet. It doesn't include any overhead of
			 * DTLS, TURN, UDP or IP headers.
			 */
			size_t mtu{ RTC::Consts::MaxSafeMtuSizeForSctp };
			/**
			 * The largest allowed message payload to be sent. Messages will be rejected
			 * if their payload is larger than this value. Note that this doesn't affect
			 * incoming messages, which may larger than this value (but smaller than
			 * `maxReceiverWindowBufferSize`).
			 */
			size_t maxMessageSize{ 256 * 1024 };
			/**
			 * The default stream priority, if not overridden by
			 * SctpSocket::SetStreamPriority(). The default value is selected to be
			 * compatible with https://www.w3.org/TR/webrtc-priority/, section 4.2-4.3.
			 */
			uint16_t defaultStreamPriority{ 256 };
			/**
			 * Maximum received window buffer size. This should be a bit larger than
			 * the largest sized message you want to be able to receive. This
			 * essentially limits the memory usage on the receive side. Note that
			 * memory is allocated dynamically, and this represents the maximum amount
			 * of buffered data. The actual memory usage of the library will be
			 * smaller in normal operation, and will be larger than this due to other
			 * allocations and overhead if the buffer is fully utilized.
			 */
			size_t maxReceiverWindowBufferSize{ 5 * 1024 * 1024 };
			/**
			 * Send queue total size limit. It will not be possible to queue more data
			 * if the queue size is larger than this number.
			 */
			size_t maxSendBufferSize{ 2000000 };
			/**
			 * Per stream send queue size limit. Similar to `maxSendBufferSize`, but
			 * limiting the size of individual streams.
			 */
			size_t perStreamSendQueueLimit{ 2000000 };
			/**
			 * A threshold that, when the amount of data in the send buffer goes below
			 * this value, will trigger Socket::OnTotalBufferedAmountLow().
			 */
			size_t totalBufferedAmountLowThreshold{ 1800000 };
			/**
			 * Max allowed RTT value. When the RTT is measured and it's found to be
			 * larger than this value, it will be discarded and not used for e.g. any
			 * RTO calculation. The default value is an extreme maximum but can be
			 * adapted to better match the environment.
			 */
			uint32_t rttMaxMs{ 60000 };
			/**
			 * Initial RTO value.
			 */
			uint32_t rtoInitialMs{ 500 };
			/**
			 * Minimum RTO value.
			 */
			uint32_t rtoMinMs{ 400 };
			/**
			 * Minimum RTO value.
			 */
			uint32_t rtoMaxMs{ 60000 };
			/**
			 * T1-init timeout (ms).
			 */
			uint64_t t1InitTimeoutMs{ 1000 };
			/**
			 * T1-cookie timeout (ms).
			 */
			uint64_t t1CookieTimeoutMs{ 1000 };
			/**
			 * T2-shutdown timeout (ms).
			 */
			uint64_t t2ShutdownTimeoutMs{ 1000 };
			/**
			 * Maximum duration of the backoff timeout. If no value is given, no
			 * limit is set.
			 */
			std::optional<uint64_t> timerMaxBackoffTimeoutMs{ std::nullopt };
			/**
			 * Hearbeat interval (on idle connections only). Set to zero to disable.
			 */
			uint64_t heartbeatIntervalMs{ 1000 };
			/**
			 * The maximum time when a SACK will be sent from the arrival of an
			 * unacknowledged Packet. Whatever is smallest of RTO/2 and this will be
			 * used.
			 */
			uint64_t delayedAckMaxTimeoutMs{ 200 };

			/**
			 * Maximum received window buffer size. It must be larger than the
			 * largest sized message we want to be able to receive.
			 *
			 * @remarks
			 * - Default value copied from dcSCTP library.
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
			 * - This feature is only enabled if both peers signal their wish to use
			 *   the same (non-zero) Zero Checksum Alternate Error Detection Method.
			 *
			 * @see RFC 9653.
			 */
			ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod zeroChecksumAlternateErrorDetectionMethod{
				ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
			};
			/**
			 * Max.Init.Retransmits. Set to std::nullopt for no limit.
			 *
			 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-16
			 */
			std::optional<size_t> maxInitRetransmits{ 8 };
			/**
			 * Maximum data retransmit attempts (for DATA, I_DATA and other Chunks).
			 * Set to std::nullopt for no limit.
			 */
			std::optional<size_t> maxRetransmits{ 8 };
		};
	} // namespace SCTP
} // namespace RTC

#endif
