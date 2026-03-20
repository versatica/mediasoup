#ifndef MS_RTC_SCTP_RETRANSMISSION_ERROR_COUNTER_HPP
#define MS_RTC_SCTP_RETRANSMISSION_ERROR_COUNTER_HPP

#include "common.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include <string_view>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The RetransmissionErrorCounter is a simple counter with a limit, and when
		 * the limit is exceeded, the counter is exhausted and the Association will
		 * be closed. It's incremented on retransmission errors, such as the T3-RTX
		 * timer expiring, but also missing heartbeats and stream reset requests.
		 */
		class RetransmissionErrorCounter
		{
		public:
			RetransmissionErrorCounter(const SctpOptions& sctpOptions);

			~RetransmissionErrorCounter();

		public:
			void Dump(int indentation = 0) const;

			/**
			 * Increments the retransmission timer. If the maximum error count has
			 * been reached, `false` will be returned.
			 */
			bool Increment(std::string_view reason);

			/**
			 * Whether maximum error count has been reached.
			 * @return [description]
			 */
			bool IsExhausted() const
			{
				return this->limit.has_value() && this->counter > this->limit.value();
			}

			/**
			 * Clears the retransmission errors.
			 */
			void Clear();

			/**
			 * Returns its current counter value.
			 */
			size_t GetCounter() const
			{
				return this->counter;
			}

		private:
			std::optional<size_t> limit{ 0 };
			size_t counter{ 0 };
		};
	} // namespace SCTP
} // namespace RTC

#endif
