#ifndef MS_RTC_REMOTE_CLOCK_OFFSET_ESTIMATOR_HPP
#define MS_RTC_REMOTE_CLOCK_OFFSET_ESTIMATOR_HPP

#include "common.hpp"
#include <vector>

namespace RTC
{
	/**
	 * Estimates the offset between the wall clock of a remote sender, as reported in
	 * the NTP field of the RTCP Sender Reports it sends, and mediasoup's own monotonic
	 * clock. Both are expressed in microseconds, so the estimated offset satisfies:
	 *
	 *   localUs = remoteUs + offsetUs
	 *
	 * Each sample is the difference between the arrival time of a Sender Report and
	 * the NTP value it carries, so it holds the clock offset plus the one way delay
	 * of that Sender Report. That delay is removed with half of the RTT when known,
	 * and the median of a sliding window is taken so that transient delay spikes are
	 * rejected.
	 *
	 * A single instance is meant to be shared by all the RTP streams of a given
	 * CNAME. Those streams come from the same machine and hence from the same wall
	 * clock, and using a different offset for each of them would reintroduce the
	 * very inter stream skew this is meant to remove.
	 *
	 * @remarks
	 * - Based on the RemoteNtpTimeEstimator class of libwebrtc.
	 */
	class RemoteClockOffsetEstimator
	{
	public:
		/**
		 * Number of most recent samples the median is computed over.
		 */
		static constexpr size_t WindowSize{ 7 };
		/**
		 * Number of samples required before an offset is reported.
		 */
		static constexpr size_t MinSampleCount{ 3 };

	public:
		RemoteClockOffsetEstimator();

	public:
		/**
		 * Feed a received RTCP Sender Report.
		 *
		 * @param remoteNtpUs - NTP field of the Sender Report, in microseconds.
		 * @param localArrivalAtUs - Our local time at which the Sender Report arrived.
		 * @param rttMs - RTT towards the sender, or 0 if not known yet.
		 */
		void AddSenderReport(int64_t remoteNtpUs, int64_t localArrivalAtUs, uint32_t rttMs);

		/**
		 * The estimated offset, or no value while less than `MinSampleCount` samples
		 * have been gathered.
		 */
		std::optional<int64_t> GetOffsetUs() const
		{
			return this->offsetUs;
		}

		/**
		 * Translate a time expressed in the remote sender's wall clock into our own
		 * monotonic clock. Returns no value if there is no offset yet or if the given
		 * time does not map into our clock.
		 */
		std::optional<int64_t> RemoteUsToLocalUs(int64_t remoteUs) const;

		void Reset();

	private:
		void UpdateOffsetUs();

	private:
		// Most recent samples, oldest first.
		std::vector<int64_t> samples;
		// Arrival time of the last accepted Sender Report, so that all the Sender
		// Reports of a same compound packet produce a single sample.
		int64_t lastLocalArrivalAtUs{ 0 };
		// Median of the samples in the window.
		std::optional<int64_t> offsetUs;
	};
} // namespace RTC

#endif
