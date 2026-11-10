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

		std::optional<Types::TransportPacketsFeedback> FeedbackAdapter::ProcessTransportWideCcFeedback(
		  const RTC::RTCP::TransportWideCcPacket* feedback, int64_t receivedAtUs)
		{
			MS_TRACE();

			if (feedback->GetPacketStatusCount() == 0)
			{
				MS_DEBUG_TAG(bwe, "feedback reports on no packet at all");

				return std::nullopt;
			}

			// A feedback without receive deltas tells which packets arrived but not
			// when, and when is what the estimation is drawn from.
			if (!feedback->HasDeltas())
			{
				MS_DEBUG_TAG(bwe, "feedback carries no receive deltas");

				return std::nullopt;
			}

			const int64_t baseTimeUs = feedback->GetBaseTimeUs();

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

			Types::TransportPacketsFeedback transportPacketsFeedback;

			transportPacketsFeedback.feedbackTimeUs = receivedAtUs;
			transportPacketsFeedback.packetFeedbacks.reserve(feedback->GetPacketStatusCount());

			size_t failedLookups{ 0 };

			feedback->ForAllRtpPackets(
			  [this, &transportPacketsFeedback, &failedLookups](
			    uint16_t wireSequenceNumber, std::optional<int64_t> deltaSinceBaseUs) -> void
			  {
				  // The wire carries the sequence number of the transport truncated to
				  // 16 bits, so it's the history, which handed it out, the one that knows
				  // which of its packets it stands for.
				  const int64_t sequenceNumber =
				    this->sendPacketHistory->UnwrapSequenceNumber(wireSequenceNumber);

				  const auto retrievedEntry =
				    this->sendPacketHistory->RetrievePacket(sequenceNumber, deltaSinceBaseUs.has_value());

				  if (!retrievedEntry.has_value())
				  {
					  ++failedLookups;

					  return;
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

				  if (deltaSinceBaseUs.has_value())
				  {
					  packetResult.receiveTimeUs = this->currentOffsetUs + deltaSinceBaseUs.value();
				  }

				  transportPacketsFeedback.packetFeedbacks.push_back(packetResult);
			  });

			if (failedLookups > 0)
			{
				MS_DEBUG_TAG(
				  bwe,
				  "could not resolve %zu of the %" PRIu16
				  " reported packets, packets reordered or history too small",
				  failedLookups,
				  feedback->GetPacketStatusCount());
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
