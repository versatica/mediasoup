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

			const auto lastSequenceNumber = this->sendPacketHistory->GetLastSequenceNumber();

			if (!lastSequenceNumber.has_value())
			{
				MS_DEBUG_TAG(bwe, "feedback received before any packet was sent");

				return std::nullopt;
			}

			const int64_t lastSentSequenceNumber = lastSequenceNumber.value();

			const int64_t baseTimeUs = feedback->GetReferenceTimestampUs();

			if (!this->lastFeedbackBaseTimeUs.has_value())
			{
				this->currentOffsetUs = receivedAtUs;
			}
			else
			{
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

			// Each packet is resolved against the one reported before it, since the
			// packets being sent may be far ahead of the ones the feedbacks report on.
			// The latest packet sent seeds it, and bounds it as well, since no report
			// can stand for a packet that was never sent.
			int64_t sequenceNumber = std::min(
			  this->lastUnwrappedSequenceNumber.value_or(lastSentSequenceNumber), lastSentSequenceNumber);

			for (const auto& packetStatus : packetStatuses)
			{
				sequenceNumber = UnwrapSequenceNumber(packetStatus.sequenceNumber, sequenceNumber);

				const auto sentPacket =
				  this->sendPacketHistory->RetrievePacket(sequenceNumber, packetStatus.received);

				if (!sentPacket.has_value())
				{
					++failedLookups;

					continue;
				}

				Types::PacketResult packetResult;

				packetResult.sentPacket = sentPacket.value();

				if (packetStatus.received)
				{
					packetResult.receiveTimeUs =
					  this->currentOffsetUs + (packetStatus.receivedAtUs - baseTimeUs);
				}

				transportPacketsFeedback.packetFeedbacks.push_back(packetResult);
			}

			this->lastUnwrappedSequenceNumber = sequenceNumber;

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

		int64_t FeedbackAdapter::UnwrapSequenceNumber(
		  uint16_t wideSequenceNumber, int64_t previousSequenceNumber)
		{
			MS_TRACE();

			// The wire carries the sequence number of the history truncated to 16 bits,
			// so the closest value to the previous one ending in those very same bits
			// is the one it stands for.
			const auto deltaSequenceNumber = static_cast<int16_t>(
			  static_cast<uint16_t>(wideSequenceNumber - static_cast<uint16_t>(previousSequenceNumber)));

			return previousSequenceNumber + deltaSequenceNumber;
		}
	} // namespace BWE
} // namespace RTC
