#define MS_CLASS "RTC::RemoteClockOffsetEstimator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RemoteClockOffsetEstimator.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RemoteClockOffsetEstimator::RemoteClockOffsetEstimator()
	{
		MS_TRACE();

		this->samples.reserve(RemoteClockOffsetEstimator::WindowSize);
	}

	void RemoteClockOffsetEstimator::AddSenderReport(
	  uint64_t remoteNtpMs, uint64_t localArrivalMs, uint32_t rttMs)
	{
		MS_TRACE();

		// Ignore Sender Reports with no NTP timestamp.
		if (remoteNtpMs == 0)
		{
			MS_DEBUG_DEV("ignoring Sender Report with no NTP timestamp");

			return;
		}

		// Ignore a Sender Report belonging to a compound packet already accounted
		// for. Otherwise a single delayed compound packet would contribute as many
		// samples as streams it reports about, and hence bias the median.
		if (localArrivalMs == this->lastLocalArrivalMs)
		{
			return;
		}

		this->lastLocalArrivalMs = localArrivalMs;

		// The sample holds the clock offset plus the one way delay of this Sender
		// Report. Assume a symmetric path and remove half of the RTT.
		const int64_t sample = static_cast<int64_t>(localArrivalMs) -
		                       static_cast<int64_t>(remoteNtpMs) - (static_cast<int64_t>(rttMs) / 2);

		if (this->samples.size() == RemoteClockOffsetEstimator::WindowSize)
		{
			this->samples.erase(this->samples.begin());
		}

		this->samples.push_back(sample);

		UpdateOffsetMs();
	}

	std::optional<uint64_t> RemoteClockOffsetEstimator::RemoteMsToLocalMs(uint64_t remoteMs) const
	{
		MS_TRACE();

		if (!this->offsetMs.has_value())
		{
			return std::nullopt;
		}

		const int64_t localMs = static_cast<int64_t>(remoteMs) + this->offsetMs.value();

		// The given time does not map into our clock, so the input is bogus.
		if (localMs < 0)
		{
			MS_WARN_2TAGS(
			  rtp, rtcp, "remote time does not map into our clock [remoteMs:%" PRIu64 "]", remoteMs);

			return std::nullopt;
		}

		return static_cast<uint64_t>(localMs);
	}

	void RemoteClockOffsetEstimator::Reset()
	{
		MS_TRACE();

		this->samples.clear();
		this->lastLocalArrivalMs = 0;
		this->offsetMs.reset();
	}

	void RemoteClockOffsetEstimator::UpdateOffsetMs()
	{
		MS_TRACE();

		if (this->samples.size() < RemoteClockOffsetEstimator::MinSampleCount)
		{
			return;
		}

		// Take the median of the window. While the window is not full its size may
		// be even, in which case the upper of the two middle samples is taken.
		std::vector<int64_t> sortedSamples(this->samples);
		const auto middle = sortedSamples.begin() + (sortedSamples.size() / 2);

		std::nth_element(sortedSamples.begin(), middle, sortedSamples.end());

		this->offsetMs = *middle;
	}
} // namespace RTC
