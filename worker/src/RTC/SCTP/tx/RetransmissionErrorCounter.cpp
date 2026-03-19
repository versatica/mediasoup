#define MS_CLASS "RTC::SCTP::RetransmissionErrorCounter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/RetransmissionErrorCounter.hpp"
#include "Logger.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		RetransmissionErrorCounter::RetransmissionErrorCounter(const SctpOptions& sctpOptions)
		  : limit(sctpOptions.maxRetransmissions)
		{
			MS_TRACE();
		}

		void RetransmissionErrorCounter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::RetransmissionErrorCounter>");

			MS_DUMP_CLEAN(
			  indentation,
			  "  limit: %s",
			  this->limit ? std::to_string(this->limit.value()).c_str() : "Infinite");
			MS_DUMP_CLEAN(indentation, "  counter: %zu", this->counter);
			MS_DUMP_CLEAN(indentation, "  exhausted: %s", IsExhausted() ? "yes" : "no");

			MS_DUMP_CLEAN(indentation, "</SCTP::RetransmissionErrorCounter>");
		}

		RetransmissionErrorCounter::~RetransmissionErrorCounter()
		{
			MS_TRACE();
		}

		bool RetransmissionErrorCounter::Increment(std::string_view reason)
		{
			MS_TRACE();

			this->counter++;

			if (IsExhausted())
			{
				MS_WARN_TAG(
				  sctp,
				  "too many retransmissions [counter:%zu, limit:%s]: %.*s",
				  this->counter,
				  this->limit ? std::to_string(this->limit.value()).c_str() : "Infinite",
				  static_cast<int>(reason.size()),
				  reason.data());

				return false;
			}

			MS_DEBUG_DEV(
			  "%.*s [counter:%zu, limit:%s]",
			  static_cast<int>(reason.size()),
			  reason.data(),
			  this->counter,
			  this->limit ? std::to_string(*this->limit).c_str() : "Infinite");

			return true;
		}

		void RetransmissionErrorCounter::Clear()
		{
			MS_TRACE();

			if (this->counter > 0)
			{
				MS_DEBUG_DEV("recovered from counter=%zu");

				this->counter = 0;
			}
		}
	} // namespace SCTP
} // namespace RTC
