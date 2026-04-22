#ifndef MS_RTC_SCTP_RETRANSMISSION_QUEUE_HPP
#define MS_RTC_SCTP_RETRANSMISSION_QUEUE_HPP

#include "common.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include "RTC/SCTP/public/AssociationListener.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The RetransmissionQueue manages all DATA/I-DATA chunks that are in-flight
		 * and schedules them to be retransmitted if necessary. Chunks are
		 * retransmitted when they have been lost for a number of consecutive SACKs,
		 * or when the retransmission timer expires.
		 *
		 * As congestion control is tightly connected with the state of transmitted
		 * packets, that's also managed here to limit the amount of data that is
		 * in-flight (sent, but not yet acknowledged).
		 */
		class RetransmissionQueue
		{
		public:
			RetransmissionQueue(
			  AssociationListener& associationListener
			  // TODO: SCTP: Implement
			  // DataTracker* dataTracker,
			  // ReassemblyQueue* reassemblyQueue,
			  // RetransmissionQueue* retransmissionQueue
			);

			// ~RetransmissionQueue() override;

		public:
			// TODO: SCTP.

		private:
			// TODO: SCTP.
		};
	} // namespace SCTP
} // namespace RTC

#endif
