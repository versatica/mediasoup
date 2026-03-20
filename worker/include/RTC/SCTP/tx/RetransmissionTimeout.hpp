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
		 * `ObserveRtt()` method is called, which calculates the retransmission
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
			 * To be called when a RTT has been measured, to update the RTO value.
			 */
			void ObserveRtt(uint64_t rtt);

			/**
			 * Returns the Retransmission Timeout (RTO) value.
			 */
			uint64_t GetRtoMs() const
			{
				return this->rtoMs;
			}

			/**
			 * Returns the smoothed RTT value.
			 */
			uint64_t GetSrttMs() const
			{
				return this->srttMs;
			}

		private:
			uint64_t minRtoMs{ 0 };
			uint64_t maxRtoMs{ 0 };
			uint64_t maxRttMs{ 0 };
			uint64_t minRttVarianceMs{ 0 };
			double srttMs{ 0 };
			double rttVarMs{ 0 };
			double rtoMs{ 0 };
			bool firstMeasurement{ false };
		};
	} // namespace SCTP
} // namespace RTC

#endif
