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
			constexpr int64_t TimeUsInfinite{ std::numeric_limits<int64_t>::max() };

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

			constexpr std::string_view bandwidthUsageToString(BandwidthUsage bandwidthUsage)
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
			 * The burst of packets a probe is sent as, and what makes its result
			 * trustworthy.
			 *
			 * A probe is only worth believing once enough of what was sent has come
			 * back, so the burst carries how many packets and how many bytes it was
			 * meant to be. Whoever measures it compares those against what actually
			 * arrived, and discards the cluster if too little of it did.
			 */
			struct ProbeCluster
			{
				/**
				 * Identifies the burst, so that the packets of one probe are not mixed
				 * with those of another.
				 */
				int64_t id{ 0 };
				/**
				 * Packets the burst was meant to be made of.
				 */
				int64_t minProbes{ 0 };
				/**
				 * Bytes the burst was meant to carry.
				 */
				int64_t minBytes{ 0 };
			};

			/**
			 * A burst that has been asked for but not sent yet.
			 *
			 * It's what whoever decides to probe hands over to whoever emits the
			 * packets: at what bitrate, for how long and in how many packets. What
			 * travels with each of those packets afterwards is a `ProbeCluster`.
			 */
			struct ProbeClusterConfig
			{
				/**
				 * Identifies the burst, and is what each of its packets carries.
				 */
				int64_t id{ 0 };
				/**
				 * Instant at which the burst was asked for.
				 */
				int64_t atUs{ 0 };
				/**
				 * Bitrate the burst is meant to be sent at (bps).
				 */
				int64_t targetBitrate{ 0 };
				/**
				 * How long the burst is meant to last.
				 */
				int64_t targetDurationUs{ 0 };
				/**
				 * Time between two consecutive bursts of packets within the probe.
				 */
				int64_t minProbeDeltaUs{ 2 * 1000 };
				/**
				 * Packets the burst is meant to be made of.
				 */
				int64_t targetProbeCount{ 0 };
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
				 * Time at which the packet was sent, or `Types::TimeUsInfinite` if it was
				 * never assigned one.
				 */
				int64_t sendTimeUs{ TimeUsInfinite };
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
				 * Data that was in flight when the packet was sent, itself included and
				 * the data that no feedback covers excluded (bytes).
				 */
				size_t dataInFlight{ 0 };
				/**
				 * Whether it's an audio packet. False for video, padding and RTX.
				 */
				bool audio{ false };
				/**
				 * The probe cluster the packet belongs to, or no value if it isn't a
				 * probe.
				 */
				std::optional<ProbeCluster> probeCluster;
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
				 * Data that was sent and no feedback has reported on yet (bytes).
				 */
				size_t dataInFlight{ 0 };
				/**
				 * Every packet the feedback reports on, received and lost alike.
				 */
				std::vector<PacketResult> packetFeedbacks;
			};
		} // namespace Types
	} // namespace BWE
} // namespace RTC

#endif
