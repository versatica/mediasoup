#define MS_CLASS "RTC::SCTP::RetransmissionTimeout"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/RetransmissionTimeout.hpp"
#include "Logger.hpp"
#include <cmath> // std::abs(), std::max(), std::round()

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		// https://datatracker.ietf.org/doc/html/rfc9260#section-16
		static constexpr double RtoAlpha{ 1.0 / 8.0 };
		static constexpr double RtoBeta{ 1.0 / 4.0 };

		// A factor that the `minRttVarianceMs` configuration option will be divided
		// by (before later multiplied with K, which is 4 according to RFC6298). When
		// this value was introduced, it was unintentionally divided by 8 since that
		// code worked with scaled numbers (to avoid floating point math). That
		// behavior is kept as downstream users have measured good values for their
		// use-cases.
		static constexpr double HeuristicVarianceAdjustment{ 8.0 };

		/* Instance methods. */

		RetransmissionTimeout::RetransmissionTimeout(const SctpOptions& sctpOptions)
		  : minRtoMs(sctpOptions.minRtoMs),
		    maxRtoMs(sctpOptions.maxRtoMs),
		    maxRttMs(sctpOptions.maxRttMs),
		    minRttVarianceMs(sctpOptions.minRttVarianceMs / HeuristicVarianceAdjustment),
		    srttMs(sctpOptions.initialRtoMs),
		    rtoMs(sctpOptions.initialRtoMs),
		    firstMeasurement(true)
		{
			MS_TRACE();
		}

		RetransmissionTimeout::~RetransmissionTimeout()
		{
			MS_TRACE();
		}

		void RetransmissionTimeout::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::RetransmissionTimeout>");
			MS_DUMP_CLEAN(indentation, "  min rto (ms): %" PRIu64, this->minRtoMs);
			MS_DUMP_CLEAN(indentation, "  max rto (ms): %" PRIu64, this->maxRtoMs);
			MS_DUMP_CLEAN(indentation, "  max rtt (ms): %" PRIu64, this->maxRttMs);
			MS_DUMP_CLEAN(indentation, "  min rtt variance (ms): %" PRIu64, this->minRttVarianceMs);
			MS_DUMP_CLEAN(indentation, "  rto (ms): %" PRIu64, GetRtoMs());
			MS_DUMP_CLEAN(indentation, "  srtt (ms): %" PRIu64, GetSrttMs());
			MS_DUMP_CLEAN(indentation, "</SCTP::RetransmissionTimeout>");
		}

		void RetransmissionTimeout::ObserveRtt(uint64_t rttMs)
		{
			MS_TRACE();

			// Unrealistic values will be skipped. If a wrongly measured (or otherwise
			// corrupt) value was processed, it could change the state in a way that
			// would take a very long time to recover.
			if (rttMs == 0 || rttMs > this->maxRttMs)
			{
				MS_WARN_TAG(sctp, "skipping given unrealistic rttMs value %" PRIu64, rttMs);

				return;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.1
			if (this->firstMeasurement)
			{
				this->srttMs           = rttMs;
				this->rttVarMs         = rttMs / 2.0;
				this->firstMeasurement = false;
			}
			else
			{
				const double rttDiffMs = std::abs(this->srttMs - static_cast<double>(rttMs));

				this->rttVarMs = ((1.0 - RtoBeta) * this->rttVarMs) + (RtoBeta * rttDiffMs);
				this->srttMs   = ((1.0 - RtoAlpha) * this->srttMs) + (RtoAlpha * rttMs);
			}

			this->rttVarMs = std::max(this->rttVarMs, static_cast<double>(this->minRttVarianceMs));
			this->rtoMs    = this->srttMs + (4.0 * this->rttVarMs);
			this->rtoMs    = std::round(
			  std::clamp(
			    this->rtoMs, static_cast<double>(this->minRtoMs), static_cast<double>(this->maxRtoMs)));

			MS_DEBUG_DEV("new computed RTO: %" PRIu64 " ms", this->rtoMs);
		}
	} // namespace SCTP
} // namespace RTC
