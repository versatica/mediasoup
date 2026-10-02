#ifndef MS_RTC_BWE_TRANSPORT_WIDE_CC_FEEDBACK_GENERATOR_HPP
#define MS_RTC_BWE_TRANSPORT_WIDE_CC_FEEDBACK_GENERATOR_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RateCalculator.hpp"
#include "RTC/SeqManager.hpp"
#include "SharedInterface.hpp"
#include <map>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Tells the remote sender when each of its packets arrived, which is what
		 * lets that sender estimate the link towards us.
		 *
		 * Nothing is estimated here. The arrival time of every packet carrying the
		 * transport wide sequence number is kept, and a feedback packet with as many
		 * of them as fit is emitted periodically. Turning those arrival times into a
		 * bitrate is the remote sender's job.
		 *
		 * How often the feedback is emitted follows the bitrate coming in, so that
		 * what reporting costs stays a small share of what is being reported on.
		 */
		class TransportWideCcFeedbackGenerator : public TimerHandleInterface::Listener
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * A feedback packet is ready for the remote sender.
				 *
				 * @param packet - Packet to send. It belongs to the caller, so the
				 *   listener must neither destroy it nor keep it around after this call
				 *   returns.
				 */
				virtual void OnTransportWideCcFeedbackGeneratorSendPacket(
				  TransportWideCcFeedbackGenerator* transportWideCcFeedbackGenerator,
				  RTC::RTCP::FeedbackRtpTransportPacket* packet) = 0;
			};

		public:
			TransportWideCcFeedbackGenerator(
			  Listener* listener, SharedInterface* shared, size_t maxRtcpPacketLen);
			~TransportWideCcFeedbackGenerator() override = default;

			TransportWideCcFeedbackGenerator(const TransportWideCcFeedbackGenerator&)            = delete;
			TransportWideCcFeedbackGenerator& operator=(const TransportWideCcFeedbackGenerator&) = delete;

			/**
			 * Feed a received RTP packet, which is ignored unless it carries the
			 * transport wide sequence number extension.
			 *
			 * @param arrivalTimeUs - Instant the packet arrived.
			 */
			void ReceiveRtpPacket(int64_t arrivalTimeUs, const RTC::RTP::Packet* packet);

#ifdef MS_TEST
		public:
#else
		private:
#endif
			/**
			 * Report everything received since the last time, in as many feedback
			 * packets as it takes.
			 */
			void FillAndSendFeedback();

			/**
			 * How often feedback should be emitted (ms) to report on the given
			 * incoming bitrate (bps) without taking more than a small share of it.
			 */
			static int64_t ComputeSendIntervalMs(int64_t incomingBitrate);

			/**
			 * How often feedback is being emitted right now (ms).
			 */
			int64_t GetSendIntervalMs() const
			{
				return this->sendIntervalMs;
			}

		private:
			/**
			 * Hand the feedback packet being filled to the listener.
			 *
			 * @returns Whether it was handed over, which does not happen when it holds
			 *   nothing that can be put on the wire.
			 */
			bool SendFeedback();

			/**
			 * Forget the arrival times too old to be reported, keeping the one the
			 * next feedback starts at.
			 */
			void MayDropOldPacketArrivalTimes(uint16_t seqNum, int64_t arrivalTimeUs);

			/**
			 * Start a new feedback packet, carrying on the given count.
			 */
			void ResetFeedbackPacket(uint8_t feedbackPacketCount);

			/**
			 * Work out how often feedback should be emitted out of the bitrate coming
			 * in, and reprogram the timer if the answer changed.
			 */
			void MayUpdateSendInterval(int64_t nowMs);

			/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
		public:
			void OnTimer(TimerHandleInterface* timer) override;

		private:
			// Largest an RTCP packet may be on this transport (bytes).
			const size_t maxRtcpPacketLen;

			// Passed by argument.
			Listener* listener{ nullptr };
			SharedInterface* shared{ nullptr };
			// Allocated by this.
			const std::unique_ptr<TimerHandleInterface> sendPeriodicTimer;
			std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket> feedbackPacket;
			// Bitrate coming in, which is what decides how often feedback is emitted.
			RTC::RtpDataCounter incomingDataCounter;
			// How often feedback is emitted (ms).
			int64_t sendIntervalMs;
			uint8_t feedbackPacketCount{ 0 };
			uint32_t feedbackMediaSsrc{ 0 };
			// Sequence number the next feedback starts reporting at, or no value
			// until the first packet is seen.
			std::optional<uint16_t> feedbackWideSeqNumStart;
			// Instant each packet arrived (us), indexed by its transport wide
			// sequence number.
			std::map<uint16_t, int64_t, RTC::SeqManager<uint16_t>::SeqLowerThan> mapPacketArrivalTimes;
		};
	} // namespace BWE
} // namespace RTC

#endif
