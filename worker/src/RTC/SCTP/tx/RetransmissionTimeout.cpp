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

		// A factor that the `minRttVarianceUs` configuration option will be divided
		// by (before later multiplied with K, which is 4 according to RFC6298). When
		// this value was introduced, it was unintentionally divided by 8 since that
		// code worked with scaled numbers (to avoid floating point math). That
		// behavior is kept as downstream users have measured good values for their
		// use-cases.
		static constexpr double HeuristicVarianceAdjustment{ 8.0 };

		/* Instance methods. */

		RetransmissionTimeout::RetransmissionTimeout(const SctpOptions& sctpOptions)
		  // NOTE: The options are in milliseconds, while this class works in
		  // microseconds so that sub-millisecond RTTs are not lost.
		  : minRtoUs(static_cast<int64_t>(sctpOptions.minRtoMs * 1000)),
		    maxRtoUs(static_cast<int64_t>(sctpOptions.maxRtoMs * 1000)),
		    maxRttUs(static_cast<int64_t>(sctpOptions.maxRttMs * 1000)),
		    minRttVarianceUs(
		      static_cast<int64_t>((sctpOptions.minRttVarianceMs * 1000) / HeuristicVarianceAdjustment)),
		    srttUs(sctpOptions.initialRtoMs * 1000),
		    rtoUs(sctpOptions.initialRtoMs * 1000),
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
			MS_DUMP_CLEAN(indentation, "  min rto (us): %" PRIi64, this->minRtoUs);
			MS_DUMP_CLEAN(indentation, "  max rto (us): %" PRIi64, this->maxRtoUs);
			MS_DUMP_CLEAN(indentation, "  max rtt (us): %" PRIi64, this->maxRttUs);
			MS_DUMP_CLEAN(indentation, "  min rtt variance (us): %" PRIi64, this->minRttVarianceUs);
			MS_DUMP_CLEAN(indentation, "  rto (us): %" PRIi64, GetRtoUs());
			MS_DUMP_CLEAN(indentation, "  srtt (us): %" PRIi64, GetSrttUs());
			MS_DUMP_CLEAN(indentation, "</SCTP::RetransmissionTimeout>");
		}

		void RetransmissionTimeout::ObserveRttUs(int64_t rttUs)
		{
			MS_TRACE();

			// Unrealistic values will be skipped. If a wrongly measured (or otherwise
			// corrupt) value was processed, it could change the state in a way that
			// would take a very long time to recover.
			if (rttUs <= 0 || rttUs > this->maxRttUs)
			{
				MS_WARN_DEV(sctp, "skipping given unrealistic rttUs value %" PRIi64, rttUs);

				return;
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.3.1
			if (this->firstMeasurement)
			{
				this->srttUs           = rttUs;
				this->rttVarUs         = rttUs / 2.0;
				this->firstMeasurement = false;
			}
			else
			{
				const double rttDiffUs = std::abs(this->srttUs - static_cast<double>(rttUs));

				this->rttVarUs = ((1.0 - RtoBeta) * this->rttVarUs) + (RtoBeta * rttDiffUs);
				this->srttUs   = ((1.0 - RtoAlpha) * this->srttUs) + (RtoAlpha * rttUs);
			}

			this->rttVarUs = std::max(this->rttVarUs, static_cast<double>(this->minRttVarianceUs));
			this->rtoUs    = this->srttUs + (4.0 * this->rttVarUs);
			this->rtoUs    = std::round(
			  std::clamp(
			    this->rtoUs, static_cast<double>(this->minRtoUs), static_cast<double>(this->maxRtoUs)));

			MS_DEBUG_DEV("new computed RTO: %" PRIi64 " us", static_cast<int64_t>(this->rtoUs));
		}
	} // namespace SCTP
} // namespace RTC
