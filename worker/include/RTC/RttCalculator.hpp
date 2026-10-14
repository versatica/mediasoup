#ifndef MS_RTC_RTT_CALCULATOR_HPP
#define MS_RTC_RTT_CALCULATOR_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "SharedInterface.hpp"
#include <vector>

namespace RTC
{
	/**
	 * Round trip time of a transport, out of the measurements its streams take
	 * one by one.
	 *
	 * @remarks
	 * - Both directions feed the same instance, since a transport is a single
	 *   remote endpoint and the way there and back is the same whoever measured
	 *   it. What each direction can measure is not the same, though: a sent
	 *   stream gets its round trip time out of every Receiver Report, while a
	 *   received one only gets it from an Extended Report carrying DLRR, which
	 *   not every endpoint sends.
	 * - What it reports is not the latest measurement but an average over the
	 *   recent ones, smoothed against the previous report, so that a single
	 *   outlier does not move it much.
	 * - It reports on its own too, since a measurement ageing out of the window
	 *   moves the average as much as a new one arriving does.
	 */
	class RttCalculator : public TimerHandleInterface::Listener
	{
	private:
		struct Measurement
		{
			/**
			 * Round trip time measured (ms).
			 */
			int64_t rttMs;
			/**
			 * Instant it was measured at (ms).
			 */
			int64_t atMs;
		};

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			/**
			 * The round trip time of the transport has moved.
			 *
			 * @param avgRttMs - What it is now.
			 */
			virtual void OnRttCalculatorRttUpdated(RttCalculator* rttCalculator, int64_t avgRttMs) = 0;
		};

	public:
		RttCalculator(Listener* listener, SharedInterface* shared);

		~RttCalculator() override = default;

		RttCalculator(const RttCalculator&)            = delete;
		RttCalculator& operator=(const RttCalculator&) = delete;

		/**
		 * Take a measurement.
		 *
		 * @param rttMs - Round trip time measured.
		 * @param nowMs - Instant it was measured at.
		 */
		void AddRtt(int64_t rttMs, int64_t nowMs);

		/**
		 * Round trip time of the transport (ms), or no value while nothing recent
		 * enough has been measured.
		 */
		std::optional<int64_t> GetAvgRttMs() const
		{
			return this->avgRttMs;
		}

		/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
	public:
		void OnTimer(TimerHandleInterface* timer) override;

	private:
		/**
		 * Drop what aged out, work out the average and tell the listener, unless
		 * nothing is left to say it with.
		 */
		void UpdateAndReport(int64_t nowMs);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		SharedInterface* shared{ nullptr };
		// Allocated by this.
		const std::unique_ptr<TimerHandleInterface> updateTimer;
		// Others.
		// Measurements still within the window, oldest first.
		std::vector<Measurement> measurements;
		std::optional<int64_t> avgRttMs;
	};
} // namespace RTC

#endif
