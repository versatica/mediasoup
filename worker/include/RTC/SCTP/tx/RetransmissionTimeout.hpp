#ifndef MS_RTC_SCTP_RETRANSMISSION_TIMEOUT_HPP
#define MS_RTC_SCTP_RETRANSMISSION_TIMEOUT_HPP

#include "common.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Manages updating of the Retransmission Timeout (RTO) SCTP variable, which
		 * is used directly as the base timeout for T3-RTX and for other timers, such
		 * as delayed ack.
		 *
		 * When a round-trip-time (RTT) is calculated (outside this class), the
		 * `ObserveRttMs()` method is called, which calculates the retransmission
		 * timeout (RTO) value. The RTO value will become larger if the RTT is high
		 * and/or the RTT values are varying a lot, which is an indicator of a bad
		 * connection.
		 */
		class RetransmissionTimeout
		{
		public:
			explicit RetransmissionTimeout(const SctpOptions& sctpOptions);

			~RetransmissionTimeout();

		public:
			void Dump(int indentation = 0) const;

			/**
			 * To be called when a RTT (us) has been measured, to update the RTO
			 * value.
			 */
			void ObserveRttUs(int64_t rttUs);

			/**
			 * Returns the Retransmission Timeout (RTO) value (us).
			 */
			int64_t GetRtoUs() const
			{
				return static_cast<int64_t>(this->rtoUs);
			}

			/**
			 * Returns the smoothed RTT value (us).
			 *
			 * @remarks
			 * - The smoothed RTT is not rounded, so the sub-microsecond part is
			 *   truncated here.
			 */
			int64_t GetSrttUs() const
			{
				return static_cast<int64_t>(this->srttUs);
			}

		private:
			int64_t minRtoUs;
			int64_t maxRtoUs;
			int64_t maxRttUs;
			int64_t minRttVarianceUs;
			double srttUs;
			double rtoUs;
			double rttVarUs{ 0 };
			bool firstMeasurement{ false };
		};
	} // namespace SCTP
} // namespace RTC

#endif
