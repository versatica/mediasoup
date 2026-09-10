#ifndef MS_RTC_BWE_FEEDBACK_ADAPTER_HPP
#define MS_RTC_BWE_FEEDBACK_ADAPTER_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/SendPacketHistory.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"

namespace RTC
{
	namespace BWE
	{
		/**
		 * Turns a feedback message into what it reports about the packets that were
		 * sent, resolved against the history of sent packets.
		 *
		 * It's the only place that knows what a feedback looks like, so that
		 * supporting another format is a matter of adding another method here.
		 *
		 * The arrival times a feedback carries come from the remote clock, against
		 * a base time that wraps around, so they are unusable as they are. The
		 * first feedback is anchored on the time it was received and every later
		 * one advances that anchor by the elapsed base time, which puts the arrival
		 * times of all of them on a single timeline. Only their differences matter
		 * downstream.
		 *
		 * @see https://datatracker.ietf.org/doc/html/draft-holmer-rmcat-transport-wide-cc-extensions
		 * @see https://datatracker.ietf.org/doc/html/rfc8888
		 */
		class FeedbackAdapter
		{
		public:
			explicit FeedbackAdapter(SendPacketHistory* sendPacketHistory);

			/**
			 * Process a transport-cc feedback.
			 *
			 * @param receivedAtUs - Time at which the feedback was received.
			 * @returns What the feedback reports about the packets that were sent, in
			 *   the order it reports them, or no value if it reports on no packet the
			 *   history still holds.
			 */
			std::optional<Types::TransportPacketsFeedback> ProcessTransportFeedback(
			  const RTCP::FeedbackRtpTransportPacket* feedback, int64_t receivedAtUs);

		private:
			/**
			 * Recover the sequence number the history gave to a packet out of the
			 * lowest bits of it that the wire carries.
			 *
			 * @param previousSequenceNumber - Sequence number the previous packet the
			 *   feedback reported on resolved into, which the returned one is the
			 *   closest to.
			 */
			static int64_t UnwrapSequenceNumber(uint16_t wideSequenceNumber, int64_t previousSequenceNumber);

		private:
			SendPacketHistory* const sendPacketHistory;
			// Our own time the arrival times of the current feedback are given
			// against.
			int64_t currentOffsetUs{ 0 };
			// Base time of the latest feedback processed, in the remote clock, or no
			// value if no feedback was ever processed.
			std::optional<int64_t> lastFeedbackBaseTimeUs;
			// Sequence number the latest packet reported on resolved into, or no
			// value if no feedback was ever processed.
			std::optional<int64_t> lastUnwrappedSequenceNumber;
		};
	} // namespace BWE
} // namespace RTC

#endif
