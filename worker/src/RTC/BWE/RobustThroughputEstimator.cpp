#define MS_CLASS "RTC::BWE::RobustThroughputEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/RobustThroughputEstimator.hpp"
#include "Logger.hpp"
#include <cmath>

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Difference between arrival times beyond which the window is dropped, since
		// such a jump means either severe reordering or that the remote clock moved.
		static constexpr int64_t MaxReorderingTimeUs{ 1000 * 1000 };
		// Smallest duration a rate is computed over, which keeps the division from
		// blowing up when every packet of the window shares a timestamp.
		static constexpr int64_t MinDurationUs{ 1000 };

		/* Instance methods. */

		RobustThroughputEstimator::RobustThroughputEstimator()
		  : RobustThroughputEstimator(RobustThroughputEstimatorOptions{})
		{
			MS_TRACE();
		}

		RobustThroughputEstimator::RobustThroughputEstimator(RobustThroughputEstimatorOptions options)
		  : options(options)
		{
			MS_TRACE();
		}

		void RobustThroughputEstimator::IncomingPacketFeedbackVector(
		  const std::vector<Types::PacketResult>& packetResults)
		{
			MS_TRACE();

			MS_ASSERT(
			  std::ranges::is_sorted(packetResults, Types::PacketResult::ReceiveTimeOrder()),
			  "packets are not sorted by arrival time");

			for (const auto& packetResult : packetResults)
			{
				// Ignore the packets without a valid send or arrival time. It shouldn't
				// happen, since the lost ones are filtered out before reaching here, but
				// handling it explicitly avoids a state that would be hard to diagnose.
				if (!packetResult.IsReceived() || packetResult.sentPacket.sendTimeUs == Types::TimeInfinite)
				{
					continue;
				}

				this->window.push_back(packetResult);

				this->window.back().sentPacket.priorUnackedData = static_cast<size_t>(std::llround(
				  static_cast<double>(this->window.back().sentPacket.priorUnackedData) *
				  this->options.unackedWeight));

				// Arrival times are usually already in order, but a reordered feedback
				// needs a few swaps to keep the window sorted.
				for (size_t idx = this->window.size() - 1;
				     idx > 0 && this->window[idx].receiveTimeUs < this->window[idx - 1].receiveTimeUs;
				     --idx)
				{
					std::swap(this->window[idx], this->window[idx - 1]);
				}

				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				const int64_t receiveDeltaUs =
				  this->window.back().receiveTimeUs.value() - packetResult.receiveTimeUs.value();

				if (receiveDeltaUs > MaxReorderingTimeUs)
				{
					MS_WARN_TAG(
					  bwe,
					  "severe packet reordering or arrival time offset changed, dropping the window"
					  " [receiveDelta:%" PRIi64 " us]",
					  receiveDeltaUs);

					this->window.clear();

					this->latestDiscardedSendTimeUs.reset();
				}
			}

			// Remove the packets that no longer belong to the window.
			while (IsFirstPacketOutsideWindow())
			{
				this->latestDiscardedSendTimeUs = std::max(
				  this->latestDiscardedSendTimeUs.value_or(this->window.front().sentPacket.sendTimeUs),
				  this->window.front().sentPacket.sendTimeUs);

				this->window.pop_front();
			}
		}

		bool RobustThroughputEstimator::IsFirstPacketOutsideWindow() const
		{
			MS_TRACE();

			if (this->window.empty())
			{
				return false;
			}

			if (this->window.size() > this->options.maxWindowPackets)
			{
				return true;
			}

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			const int64_t currentWindowDurationUs =
			  this->window.back().receiveTimeUs.value() - this->window.front().receiveTimeUs.value();

			if (currentWindowDurationUs > this->options.maxWindowDurationUs)
			{
				return true;
			}

			return this->window.size() > this->options.windowPackets &&
			       currentWindowDurationUs > this->options.minWindowDurationUs;
		}

		std::optional<int64_t> RobustThroughputEstimator::GetBitrate() const
		{
			MS_TRACE();

			if (this->window.empty() || this->window.size() < this->options.requiredPackets)
			{
				return std::nullopt;
			}

			int64_t largestRecvGapUs{ 0 };
			int64_t secondLargestRecvGapUs{ 0 };

			for (size_t idx = 1; idx < this->window.size(); ++idx)
			{
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				const int64_t gapUs =
				  this->window[idx].receiveTimeUs.value() - this->window[idx - 1].receiveTimeUs.value();

				if (gapUs > largestRecvGapUs)
				{
					secondLargestRecvGapUs = largestRecvGapUs;
					largestRecvGapUs       = gapUs;
				}
				else if (gapUs > secondLargestRecvGapUs)
				{
					secondLargestRecvGapUs = gapUs;
				}
			}

			std::optional<int64_t> firstSendTimeUs;
			std::optional<int64_t> lastSendTimeUs;
			std::optional<int64_t> firstRecvTimeUs;
			std::optional<int64_t> lastRecvTimeUs;
			int64_t recvSize{ 0 };
			int64_t sendSize{ 0 };
			int64_t firstRecvSize{ 0 };
			int64_t lastSendSize{ 0 };
			size_t numSentPacketsInWindow{ 0 };

			for (const auto& packetResult : this->window)
			{
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				const int64_t receiveTimeUs = packetResult.receiveTimeUs.value();

				if (!firstRecvTimeUs.has_value() || receiveTimeUs < firstRecvTimeUs.value())
				{
					firstRecvTimeUs = receiveTimeUs;
					firstRecvSize   = packetResult.sentPacket.size + packetResult.sentPacket.priorUnackedData;
				}

				lastRecvTimeUs = std::max(lastRecvTimeUs.value_or(receiveTimeUs), receiveTimeUs);

				recvSize += packetResult.sentPacket.size;
				recvSize += packetResult.sentPacket.priorUnackedData;

				// Packets sent before something that was already dropped from the window
				// were reordered, and their send time may be so far in the past that it
				// would drag the send rate down. Leave them out of it.
				if (
				  this->latestDiscardedSendTimeUs.has_value() &&
				  packetResult.sentPacket.sendTimeUs < this->latestDiscardedSendTimeUs.value())
				{
					continue;
				}

				if (!lastSendTimeUs.has_value() || packetResult.sentPacket.sendTimeUs > lastSendTimeUs.value())
				{
					lastSendTimeUs = packetResult.sentPacket.sendTimeUs;
					lastSendSize   = packetResult.sentPacket.size + packetResult.sentPacket.priorUnackedData;
				}

				firstSendTimeUs = std::min(
				  firstSendTimeUs.value_or(packetResult.sentPacket.sendTimeUs),
				  packetResult.sentPacket.sendTimeUs);

				sendSize += packetResult.sentPacket.size;
				sendSize += packetResult.sentPacket.priorUnackedData;

				++numSentPacketsInWindow;
			}

			// Suppose a packet of size S is sent every T microseconds. A window of N
			// packets holds N*S bytes, but the time between the first and the last one
			// is only (N-1)*T, so one packet has to be left out to get the rate S/T.
			// Which one depends on the direction: the receive rate doesn't depend on
			// the size of the first packet, and the send rate doesn't depend on the
			// size of the last one.
			recvSize -= firstRecvSize;
			sendSize -= lastSendSize;

			// Replace the largest gap by the second largest one, so that a short pause
			// followed by a burst of delayed packets doesn't drop the estimate. That
			// can overestimate, which is guarded against by never returning more than
			// the send rate.
			MS_ASSERT(firstRecvTimeUs.has_value(), "no arrival time in a non empty window");
			MS_ASSERT(lastRecvTimeUs.has_value(), "no arrival time in a non empty window");

			const int64_t recvDurationUs = std::max(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  (lastRecvTimeUs.value() - firstRecvTimeUs.value()) - largestRecvGapUs + secondLargestRecvGapUs,
			  MinDurationUs);

			if (numSentPacketsInWindow < this->options.requiredPackets)
			{
				// Too few send times to compute a reliable send rate.
				return recvSize * 8 * 1000000 / recvDurationUs;
			}

			MS_ASSERT(firstSendTimeUs.has_value(), "no send time although packets were counted");
			MS_ASSERT(lastSendTimeUs.has_value(), "no send time although packets were counted");

			const int64_t sendDurationUs = std::max(
			  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			  lastSendTimeUs.value() - firstSendTimeUs.value(),
			  MinDurationUs);

			return std::min(
			  sendSize * 8 * 1000000 / sendDurationUs, recvSize * 8 * 1000000 / recvDurationUs);
		}
	} // namespace BWE
} // namespace RTC
