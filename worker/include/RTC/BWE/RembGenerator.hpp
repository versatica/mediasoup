#ifndef MS_RTC_BWE_REMB_GENERATOR_HPP
#define MS_RTC_BWE_REMB_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Tells the remote sender how much it may send towards us, as a REMB.
		 *
		 * Two things end up in that single number: what the incoming link is
		 * estimated to bear, and the cap the application asked for.
		 */
		class RembGenerator
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * A REMB packet is ready for the remote sender.
				 *
				 * @param packet - Packet to send. It belongs to the caller, which
				 *   destroys it once this call returns, so it must be neither kept nor
				 *   deleted here.
				 */
				virtual void OnRembGeneratorSendPacket(
				  RembGenerator* rembGenerator, RTC::RTCP::FeedbackPsRembPacket* packet) = 0;
			};

		public:
			explicit RembGenerator(Listener* listener);

			RembGenerator(const RembGenerator&)            = delete;
			RembGenerator& operator=(const RembGenerator&) = delete;

			/**
			 * Feed a new estimation of what the incoming link bears (bps).
			 *
			 * @param nowMs - Current instant.
			 * @param ssrcs - Streams the estimation applies to.
			 *
			 * @remarks
			 * - A REMB goes out when the estimation drops enough to be worth telling
			 *   right away, and otherwise no more often than every 200 ms.
			 */
			void OnReceiveBitrateChanged(int64_t nowMs, const std::vector<uint32_t>& ssrcs, int64_t bitrate);

			/**
			 * Cap what is announced to the remote sender (bps), or no value to
			 * announce whatever is estimated.
			 *
			 * @param nowMs - Current instant.
			 */
			void SetMaxIncomingBitrate(int64_t nowMs, std::optional<int64_t> bitrate);

			/**
			 * Announce the cap again, or that it is gone, which is what gets either
			 * across when there is no estimation driving anything.
			 *
			 * @param nowMs - Current instant.
			 */
			void MaySendLimitationRembFeedback(int64_t nowMs);

		private:
			/**
			 * Build a REMB carrying the given bitrate and hand it to the listener.
			 */
			void SendRemb(int64_t bitrate, const std::vector<uint32_t>& ssrcs);

		private:
			// Passed by argument.
			Listener* listener{ nullptr };
			// Others.
			// Instant the latest REMB was sent at, or no value until one is.
			std::optional<int64_t> lastRembSentAtMs;
			// Bitrate the latest REMB carried, or no value until one is sent. Zero is
			// a value of its own, since that is how a REMB says there is no limit.
			std::optional<int64_t> lastSentRembBitrate;
			// Most that may be announced, or no value when there is no cap.
			std::optional<int64_t> maxIncomingBitrate;
			// Instant the cap was last announced at, or no value until it is.
			std::optional<int64_t> limitationRembSentAtMs;
			// How many REMBs announcing that the cap is gone are still owed.
			uint8_t unlimitedRembCounter{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
