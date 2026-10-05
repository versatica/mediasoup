#ifndef MS_RTC_BWE_PACKET_LOSS_TRACKER_HPP
#define MS_RTC_BWE_PACKET_LOSS_TRACKER_HPP

#include "common.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Turns the RTCP Receiver Reports that the remote endpoint sends about our
		 * outgoing streams into how many packets were lost and how many were
		 * expected since the previous report.
		 *
		 * A report block carries running totals rather than what happened lately, so
		 * what a report says on its own means nothing: the figures only become a
		 * measurement once they are subtracted from those of the previous report of
		 * the same stream. The whole Receiver Report is taken at once and the deltas
		 * of all its blocks are added together, since what the rate control reasons
		 * about is the loss of the transport and not that of a single stream.
		 */
		class PacketLossTracker
		{
		public:
			/**
			 * How many packets the remote endpoint missed and how many it should have
			 * got, over the time between the previous Receiver Report and this one.
			 */
			struct Loss
			{
				/**
				 * Packets lost. It may be negative, since the running total a report
				 * carries goes down when duplicates arrive.
				 */
				int64_t lostPackets;
				/**
				 * Packets expected, which is what the sequence numbers covered. It may
				 * also be negative, when a report built before the previous one still
				 * arrives after it and its sequence numbers go backwards.
				 */
				int64_t expectedPackets;
			};

		private:
			/**
			 * The running totals of the latest report of a stream, which is what the
			 * next one is subtracted from.
			 */
			struct ReportTotals
			{
				/**
				 * Highest sequence number the remote endpoint had seen, extended with
				 * the count of cycles it had wrapped.
				 */
				uint32_t extendedHighestSequenceNumber;
				/**
				 * Packets it had missed since the stream began.
				 */
				int32_t cumulativeLost;
			};

		public:
			/**
			 * Feed a received RTCP Receiver Report.
			 *
			 * @returns What was lost and expected since the previous report, or no
			 *   value when this one says nothing new.
			 *
			 * @remarks
			 * - The first report of a stream never produces a value: it only takes
			 *   down the totals that the next one is measured against.
			 */
			std::optional<Loss> ReceiveReceiverReport(RTC::RTCP::ReceiverReportPacket* packet);

			/**
			 * Forget a stream, which is what the closing of whatever was sending it
			 * calls for. Its totals would otherwise be kept for as long as the
			 * transport lives, and a stream that comes back reusing the SSRC would be
			 * measured against the totals of the one that is gone.
			 */
			void RemoveStream(uint32_t ssrc);

		private:
			// Latest totals of each stream, by SSRC.
			ankerl::unordered_dense::map<uint32_t, ReportTotals> mapSsrcReportTotals;
		};
	} // namespace BWE
} // namespace RTC

#endif
