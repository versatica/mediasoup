#define MS_CLASS "RTC::BWE::FeedbackAdapter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/FeedbackAdapter.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		FeedbackAdapter::FeedbackAdapter(SendPacketHistory* sendPacketHistory)
		  : sendPacketHistory(sendPacketHistory)
		{
			MS_TRACE();
		}

		std::optional<Types::TransportPacketsFeedback> FeedbackAdapter::ProcessTransportFeedback(
		  const RTCP::FeedbackRtpTransportPacket* feedback, int64_t receivedAtUs)
		{
			MS_TRACE();

			if (feedback->GetPacketStatusCount() == 0)
			{
				MS_DEBUG_TAG(bwe, "feedback reports on no packet at all");

				return std::nullopt;
			}

			const int64_t baseTimeUs = feedback->GetReferenceTimestampUs();

			if (!this->lastFeedbackBaseTimeUs.has_value())
			{
				this->currentOffsetUs = receivedAtUs;
			}
			else
			{
				// NOTE: Taken in microseconds and not quantized any further. The groups
				// whose arrival times the delay detector compares are only a few
				// milliseconds apart, so rounding them to whole milliseconds would be
				// noise of the same order as the slope being measured.
				const int64_t baseDeltaUs = feedback->GetBaseDeltaUs(this->lastFeedbackBaseTimeUs.value());

				if (baseDeltaUs < -this->currentOffsetUs)
				{
					MS_DEBUG_TAG(
					  bwe,
					  "base time of the feedback moved too far back, anchoring it again"
					  " [baseDelta:%" PRIi64 " us]",
					  baseDeltaUs);

					this->currentOffsetUs = receivedAtUs;
				}
				else
				{
					this->currentOffsetUs += baseDeltaUs;
				}
			}

			this->lastFeedbackBaseTimeUs = baseTimeUs;

			const auto packetStatuses = feedback->GetPacketStatuses();

			Types::TransportPacketsFeedback transportPacketsFeedback;

			transportPacketsFeedback.feedbackTimeUs = receivedAtUs;
			transportPacketsFeedback.packetFeedbacks.reserve(packetStatuses.size());

			size_t failedLookups{ 0 };

			for (const auto& packetStatus : packetStatuses)
			{
				// The wire carries the sequence number of the transport truncated to 16
				// bits, so it's the history, which handed it out, the one that knows
				// which of its packets it stands for.
				const int64_t sequenceNumber =
				  this->sendPacketHistory->UnwrapSequenceNumber(packetStatus.sequenceNumber);

				const auto retrievedEntry =
				  this->sendPacketHistory->RetrievePacket(sequenceNumber, packetStatus.received);

				if (!retrievedEntry.has_value())
				{
					++failedLookups;

					continue;
				}

				const auto& entry = retrievedEntry.value();

				Types::PacketResult packetResult;

				packetResult.sentPacket = entry.sentPacket;
				packetResult.rtpPacketInfo =
				  Types::PacketResult::RtpPacketInfo{ .ssrc              = entry.ssrc,
					                                    .rtpSequenceNumber = entry.seq,
					                                    .isRetransmission  = entry.isRetransmission };
				packetResult.ambiguousReceiveTime = entry.ambiguousReceiveTime;
				packetResult.sentWithEct1         = entry.sentWithEct1;

				if (packetStatus.received)
				{
					packetResult.receiveTimeUs =
					  this->currentOffsetUs + (packetStatus.receivedAtUs - baseTimeUs);
				}

				transportPacketsFeedback.packetFeedbacks.push_back(packetResult);
			}

			if (failedLookups > 0)
			{
				MS_DEBUG_TAG(
				  bwe,
				  "could not resolve %zu of the %zu reported packets, packets reordered or history too small",
				  failedLookups,
				  packetStatuses.size());
			}

			if (transportPacketsFeedback.packetFeedbacks.empty())
			{
				return std::nullopt;
			}

			transportPacketsFeedback.dataInFlight = this->sendPacketHistory->GetOutstandingBytes();

			return transportPacketsFeedback;
		}

	} // namespace BWE
} // namespace RTC
