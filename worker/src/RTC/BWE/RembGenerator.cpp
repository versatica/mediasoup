#define MS_CLASS "RTC::BWE::RembGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/RembGenerator.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// How often a REMB may be sent when the estimation only goes up or barely
		// moves.
		static constexpr int64_t RembSendIntervalMs{ 200 };
		// How much lower than the latest one an estimation has to be for its REMB to
		// go out without waiting, as a percentage.
		static constexpr int64_t SendThresholdPercent{ 103 };
		// How long the cap may go without being stated, either by a REMB of its own
		// or by one carrying an estimation that already honours it.
		static constexpr int64_t LimitationRembIntervalMs{ 1500 };
		// How many REMBs are sent to announce that the cap is gone, since losing the
		// only one would leave the remote sender limited forever.
		static constexpr uint8_t UnlimitedRembNumPackets{ 4 };

		/* Instance methods. */

		RembGenerator::RembGenerator(Listener* listener) : listener(listener)
		{
			MS_TRACE();
		}

		void RembGenerator::OnReceiveBitrateChanged(
		  int64_t nowMs, const std::vector<uint32_t>& ssrcs, int64_t bitrate)
		{
			MS_TRACE();

			if (
			  this->lastSentRembBitrate.has_value() && this->lastRembSentAtMs.has_value() &&
			  (bitrate * SendThresholdPercent) / 100 > this->lastSentRembBitrate.value() &&
			  nowMs - this->lastRembSentAtMs.value() < RembSendIntervalMs)
			{
				return;
			}

			this->lastRembSentAtMs    = nowMs;
			this->lastSentRembBitrate = bitrate;

			if (this->maxIncomingBitrate.has_value())
			{
				bitrate = std::min(bitrate, this->maxIncomingBitrate.value());
			}

			SendRemb(bitrate, ssrcs);
		}

		void RembGenerator::SetMaxIncomingBitrate(int64_t nowMs, std::optional<int64_t> bitrate)
		{
			MS_TRACE();

			// A cap of zero cannot be told apart from no cap at all, since that is what
			// a REMB of zero means on the wire, and a negative one means nothing. Both
			// leave the cap as it was rather than turning into its opposite.
			if (bitrate.has_value() && bitrate.value() <= 0)
			{
				MS_WARN_TAG(
				  bwe, "ignoring invalid max incoming bitrate [bitrate:%" PRIi64 "]", bitrate.value());

				return;
			}

			const bool wasLimited = this->maxIncomingBitrate.has_value();

			this->maxIncomingBitrate = bitrate;

			if (!this->maxIncomingBitrate.has_value())
			{
				if (wasLimited)
				{
					this->unlimitedRembCounter = UnlimitedRembNumPackets;

					MaySendLimitationRembFeedback(nowMs);
				}

				return;
			}

			// The cap is back, so the REMBs that were still owed to announce that it
			// was gone no longer say anything true.
			this->unlimitedRembCounter = 0;

			// Nothing to tell if a REMB went out recently carrying a bitrate that the
			// new cap already allows.
			// NOTE: The instant is taken down either way, since what is already on the
			// wire honours the cap just as well, and otherwise the next incoming
			// packet would announce it again right behind that REMB.
			this->limitationRembSentAtMs = nowMs;

			if (
			  this->lastRembSentAtMs.has_value() &&
			  nowMs - this->lastRembSentAtMs.value() < RembSendIntervalMs &&
			  this->lastSentRembBitrate.has_value() && this->lastSentRembBitrate.value() != 0 &&
			  this->lastSentRembBitrate.value() <= this->maxIncomingBitrate.value())
			{
				return;
			}

			SendRemb(this->maxIncomingBitrate.value(), {});
		}

		void RembGenerator::MaySendLimitationRembFeedback(int64_t nowMs)
		{
			MS_TRACE();

			const bool announcingRemoval = this->unlimitedRembCounter > 0;

			if (!announcingRemoval && !this->maxIncomingBitrate.has_value())
			{
				return;
			}

			// A REMB carrying the estimation is already no higher than the cap, so
			// while those keep flowing the cap needs no announcement of its own.
			if (!announcingRemoval && this->lastRembSentAtMs.has_value() && nowMs - this->lastRembSentAtMs.value() <= LimitationRembIntervalMs)
			{
				return;
			}

			const bool dueAgain = !this->limitationRembSentAtMs.has_value() ||
			                      nowMs - this->limitationRembSentAtMs.value() > LimitationRembIntervalMs;

			// The first of the REMBs announcing that the cap is gone goes out without
			// waiting.
			if (!dueAgain && this->unlimitedRembCounter != UnlimitedRembNumPackets)
			{
				return;
			}

			const int64_t bitrate = this->maxIncomingBitrate.value_or(0);

			MS_DEBUG_DEV("sending limitation REMB [bitrate:%" PRIi64 "]", bitrate);

			this->limitationRembSentAtMs = nowMs;

			if (this->unlimitedRembCounter > 0)
			{
				this->unlimitedRembCounter--;
			}

			SendRemb(bitrate, {});
		}

		void RembGenerator::SendRemb(int64_t bitrate, const std::vector<uint32_t>& ssrcs)
		{
			MS_TRACE();

			RTC::RTCP::FeedbackPsRembPacket packet(uint32_t{ 0 }, uint32_t{ 0 });

			packet.SetBitrate(bitrate);

			if (!ssrcs.empty())
			{
				packet.SetSsrcs(ssrcs);
			}

			this->listener->OnRembGeneratorSendRemb(this, std::addressof(packet));
		}
	} // namespace BWE
} // namespace RTC
