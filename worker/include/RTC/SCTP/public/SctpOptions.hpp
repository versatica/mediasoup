#ifndef MS_RTC_SCTP_OPTIONS_HPP
#define MS_RTC_SCTP_OPTIONS_HPP

#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP options.
		 */
		struct SctpOptions
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
			 * Maximum size of an SCTP Packet. It doesn't include any overhead of
			 * DTLS, TURN, UDP or IP headers.
			 */
			size_t mtu{ RTC::Consts::MaxSafeMtuSizeForSctp };

			/**
			 * The largest allowed message payload to be sent. Messages will be rejected
			 * if their payload is larger than this value. Note that this doesn't affect
			 * incoming messages, which may larger than this value (but smaller than
			 * `maxReceiverWindowBufferSize`).
			 */
			size_t maxSendMessageSize{ 256 * 1024 };

			/**
			 * The default stream priority, if not overridden by
			 * `Association::SetStreamPriority()`. The default value is selected to be
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
			 * this value, will trigger `Association::OnAssociationTotalBufferedAmountLow()`.
			 */
			size_t totalBufferedAmountLowThreshold{ 1800000 };

			/**
			 * Max allowed RTT value. When the RTT is measured and it's found to be
			 * larger than this value, it will be discarded and not used for e.g. any
			 * RTO calculation. The default value is an extreme maximum but can be
			 * adapted to better match the environment.
			 */
			uint64_t maxRttMs{ 60000 };

			/**
			 * Initial RTO value.
			 */
			uint64_t initialRtoMs{ 500 };

			/**
			 * Minimum RTO value.
			 */
			uint64_t minRtoMs{ 400 };

			/**
			 * Minimum RTO value.
			 */
			uint64_t maxRtoMs{ 60000 };

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
			uint64_t heartbeatIntervalMs{ 30000 };

			/**
			 * The maximum time when a SACK will be sent from the arrival of an
			 * unacknowledged Packet. Whatever is smallest of RTO/2 and this will be
			 * used.
			 */
			uint64_t delayedAckMaxTimeoutMs{ 200 };

			/**
			 * The minimum limit for the measured RTT variance.
			 *
			 * Setting this below the expected delayed ack timeout (+ margin) of the
			 * peer might result in unnecessary retransmissions, as the maximum time
			 * it takes to ACK a DATA chunk is typically RTT + ATO (delayed ack
			 * timeout), and when the SCTP channel is quite idle, and heartbeats
			 * dominate the source of RTT measurement, the RTO would converge with the
			 * smoothed RTT (SRTT). The default ATO is 200ms in usrsctp, and a 20ms
			 * (10%) margin would include the processing time of received packets and
			 * the clock granularity when setting the delayed ack timer on the peer.
			 *
			 * This is defined as "G" in the algorithm for TCP in
			 * https://datatracker.ietf.org/doc/html/rfc6298#section-4.
			 */
			uint64_t minRttVarianceMs{ 220 };

			/**
			 * The initial congestion window size, in number of MTUs.
			 *
			 * @see https://tools.ietf.org/html/rfc4960#section-7.2.1 which defaults
			 * at ~3 and https://research.google/pubs/pub36640/ which argues for at
			 * least ten segments.
			 */
			size_t initialCwndMtus{ 10 };

			/**
			 * The minimum congestion window size, in number of MTUs, upon detection
			 * of/ packet loss by SACK. Note that if the retransmission timer expires,
			 * the congestion window will be as small as one MTU.
			 *
			 * @see https://tools.ietf.org/html/rfc4960#section-7.2.3.
			 */
			size_t minCwndMtus{ 4 };

			/**
			 * When the congestion window is at or above this number of MTUs, the
			 * congestion control algorithm will avoid filling the congestion window
			 * fully, if that results in fragmenting large messages into quite small
			 * packets. When the congestion window is smaller than this option, it
			 * will aim to fill the congestion window as much as it can, even if it
			 * results in creating small fragmented packets.
			 */
			size_t avoidFragmentationCwndMtus{ 6 };

			/**
			 * When the congestion window is below this number of MTUs, sent data
			 * chunks will have the "I" (Immediate SACK - RFC7053) bit set. That will
			 * prevent the receiver from delaying the SACK, which result in shorter
			 * time until the sender can send the next packet as its driven by SACKs.
			 * This can reduce latency for low utilized and lossy connections.
			 *
			 * Default value set to be same as initial congestion window. Set to zero
			 * to disable.
			 */
			size_t immediateSackUnderCwndMtus{ 10 };

			/**
			 * The number of packets that may be sent at once. This is limited to
			 * avoid bursts that too quickly fill the send buffer. Typically in a
			 * connection in its "slow start" phase (when it sends as much as it can),
			 * it will send up to three packets for every SACK received, so the default
			 * limit is set just above that, and then mostly applicable for (but not
			 * limited to) fast retransmission scenarios.
			 */
			size_t maxBurst{ 4 };

			/**
			 * Maximum data retransmit attempts (for DATA, I_DATA and other Chunks).
			 * Set to std::nullopt for no limit.
			 */
			std::optional<size_t> maxRetransmissions{ 10 };

			/**
			 * Max.Init.Retransmits. Set to std::nullopt for no limit.
			 *
			 * @see https://datatracker.ietf.org/doc/html/rfc9260#section-16
			 */
			std::optional<size_t> maxInitRetransmissions{ 8 };

			/**
			 * Enable Partial Reliability Extension.
			 * @see RFC 3758.
			 */
			bool enablePartialReliability{ true };

			/**
			 * Enable Stream Schedulers and User Message Interleaving (I-DATA Chunks).
			 *
			 * @see RFC 8260.
			 */
			bool enableMessageInterleaving{ true };

			/**
			 * Whether RTO should be added to heartbeat interval.
			 */
			bool heartbeatIntervalIncludeRtt{ true };

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
		};

		/**
		 * Send options given when sending SCTP messages.
		 */
		struct SendMessageOptions
		{
			/**
			 * Whether the message should be sent with unordered message delivery.
			 */
			bool unordered{ false };

			/**
			 * If set, will discard messages that haven't been correctly sent and
			 * received before the lifetime has expired. This is only available if
			 * the peer supports Partial Reliability Extension (RFC 3758).
			 */
			std::optional<uint64_t> lifetime{ std::nullopt };

			/**
			 * If set, limits the number of retransmissions. This is only available
			 * if the peer supports Partial Reliability Extension (RFC 3758).
			 */
			std::optional<size_t> maxRetransmissions{ std::nullopt };

			/**
			 * If set, will generate lifecycle events for this message. See e.g.
			 * `AssociationListener::OnAssociationLifecycleMessageFullySent()`. This
			 * value is decided by the application and the library will provide it to
			 * all lifecycle callbacks.
			 */
			std::optional<uint64_t> lifecycleId{ std::nullopt };
		};
	} // namespace SCTP
} // namespace RTC

#endif
