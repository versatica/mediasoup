#ifndef MS_RTC_BWE_BWE_TYPES_HPP
#define MS_RTC_BWE_BWE_TYPES_HPP

#include "common.hpp"
#include <limits>
#include <string_view>
#include <vector>

namespace RTC
{
	namespace BWE
	{
		namespace Types
		{
			/**
			 * Bitrate meaning that there is no limit at all (bps).
			 */
			constexpr int64_t BitrateInfinite{ std::numeric_limits<int64_t>::max() };

			/**
			 * Time meaning that no time was ever assigned.
			 */
			constexpr int64_t TimeInfinite{ std::numeric_limits<int64_t>::max() };

			/**
			 * How the network is behaving according to the delay based detector.
			 */
			enum class BandwidthUsage : uint8_t
			{
				/**
				 * Queues are not growing.
				 */
				NORMAL,
				/**
				 * Queues are being emptied, so we are sending below the link capacity.
				 */
				UNDERUSING,
				/**
				 * Queues are filling up, so we are sending above the link capacity.
				 */
				OVERUSING
			};

			constexpr std::string_view BandwidthUsageToString(BandwidthUsage bandwidthUsage)
			{
				switch (bandwidthUsage)
				{
					case BandwidthUsage::NORMAL:
					{
						return "NORMAL";
					}

					case BandwidthUsage::UNDERUSING:
					{
						return "UNDERUSING";
					}

					case BandwidthUsage::OVERUSING:
					{
						return "OVERUSING";
					}

						NO_DEFAULT_GCC();
				}
			}

			/**
			 * Estimate of the capacity of the link, used to bound the rate control.
			 */
			struct NetworkStateEstimate
			{
				/**
				 * Safe measure of the available capacity (bps).
				 */
				std::optional<int64_t> linkCapacityLower;
				/**
				 * Limit applied when increasing the bitrate (bps).
				 */
				std::optional<int64_t> linkCapacityUpper;
			};

			/**
			 * Input given to the rate control on every update.
			 */
			struct RateControlInput
			{
				/**
				 * How the network is behaving according to the delay based detector.
				 */
				BandwidthUsage bandwidthUsage;
				/**
				 * Bitrate acknowledged by the receiver (bps), or no value if it could
				 * not be measured.
				 */
				std::optional<int64_t> estimatedThroughput;
			};

			/**
			 * A packet that was sent and is being tracked until its feedback arrives.
			 */
			struct SentPacket
			{
				/**
				 * Sequence number of the packet within the transport, unique over the
				 * whole transport and increasing by one per tracked packet.
				 *
				 * @remarks
				 * - It's independent of the feedback format. With transport-cc it
				 *   matches the wide sequence number carried in the header extension,
				 *   while with RFC 8888 the feedback identifies packets by SSRC and RTP
				 *   sequence number and the adapter resolves them against this one.
				 */
				int64_t sequenceNumber;
				/**
				 * Time at which the packet was sent, or `Types::TimeInfinite` if it was
				 * never assigned one.
				 */
				int64_t sendTimeUs{ TimeInfinite };
				/**
				 * Size of the packet including overhead up to the IP layer (bytes).
				 */
				size_t size{ 0 };
				/**
				 * Size of the preceding packets that are not part of any feedback
				 * (bytes).
				 */
				size_t priorUnackedData{ 0 };
				/**
				 * Whether it's an audio packet. False for video, padding and RTX.
				 */
				bool audio{ false };
			};

			/**
			 * What a feedback reported about a single sent packet.
			 */
			struct PacketResult
			{
				/**
				 * Orders packets by the time at which they arrived.
				 *
				 * @remarks
				 * - Send time and sequence number break the ties, which are common
				 *   because the arrival times within a burst are reported with a coarse
				 *   resolution.
				 */
				struct ReceiveTimeOrder
				{
					bool operator()(const PacketResult& lhs, const PacketResult& rhs) const;
				};

				/**
				 * Whether the packet reached the receiver at all.
				 */
				bool IsReceived() const
				{
					return this->receiveTimeUs.has_value();
				}

				SentPacket sentPacket;
				/**
				 * Time at which the packet arrived, in the remote clock reference, or no
				 * value if it was reported as lost.
				 */
				std::optional<int64_t> receiveTimeUs;
			};

			/**
			 * A whole feedback message, reporting on the packets sent so far.
			 */
			struct TransportPacketsFeedback
			{
				/**
				 * The packets that reached the receiver, ordered by arrival time.
				 *
				 * @remarks
				 * - Send time and sequence number break the ties, which are common
				 *   because the arrival times within a burst are reported with a coarse
				 *   resolution.
				 */
				std::vector<PacketResult> SortedByReceiveTime() const;

				/**
				 * The packets that reached the receiver, in the order they were
				 * reported.
				 */
				std::vector<PacketResult> ReceivedWithSendInfo() const;

				/**
				 * Time at which this feedback was received, in our own clock reference.
				 */
				int64_t feedbackTimeUs;
				/**
				 * Every packet the feedback reports on, received and lost alike.
				 */
				std::vector<PacketResult> packetFeedbacks;
			};
		} // namespace Types
	} // namespace BWE
} // namespace RTC

#endif
