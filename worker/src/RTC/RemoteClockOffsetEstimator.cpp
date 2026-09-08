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
	  int64_t remoteNtpUs, int64_t localArrivalAtUs, int64_t rttMs)
	{
		MS_TRACE();

		// Ignore Sender Reports with no NTP timestamp.
		if (remoteNtpUs == 0)
		{
			MS_DEBUG_DEV("ignoring Sender Report with no NTP timestamp");

			return;
		}

		// Ignore a Sender Report belonging to a compound packet already accounted
		// for. Otherwise a single delayed compound packet would contribute as many
		// samples as streams it reports about, and hence bias the median.
		if (localArrivalAtUs == this->lastLocalArrivalAtUs)
		{
			return;
		}

		this->lastLocalArrivalAtUs = localArrivalAtUs;

		// The sample holds the clock offset plus the one way delay of this Sender
		// Report. Assume a symmetric path and remove half of the RTT.
		//
		// NOTE: The RTT is given in milliseconds, hence the conversion.
		const int64_t sample = localArrivalAtUs - remoteNtpUs - ((rttMs * 1000) / 2);

		if (this->samples.size() == RemoteClockOffsetEstimator::WindowSize)
		{
			this->samples.erase(this->samples.begin());
		}

		this->samples.push_back(sample);

		UpdateOffsetUs();
	}

	std::optional<int64_t> RemoteClockOffsetEstimator::RemoteUsToLocalUs(int64_t remoteUs) const
	{
		MS_TRACE();

		if (!this->offsetUs.has_value())
		{
			return std::nullopt;
		}

		const int64_t localUs = remoteUs + this->offsetUs.value();

		// The given time does not map into our clock, so the input is bogus.
		if (localUs < 0)
		{
			MS_WARN_2TAGS(
			  rtp, rtcp, "remote time does not map into our clock [remoteUs:%" PRIi64 "]", remoteUs);

			return std::nullopt;
		}

		return localUs;
	}

	void RemoteClockOffsetEstimator::Reset()
	{
		MS_TRACE();

		this->samples.clear();
		this->lastLocalArrivalAtUs = 0;
		this->offsetUs.reset();
	}

	void RemoteClockOffsetEstimator::UpdateOffsetUs()
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

		this->offsetUs = *middle;
	}
} // namespace RTC
