#define MS_CLASS "RTC::BWE::TransportCcFeedbackGenerator"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/TransportCcFeedbackGenerator.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Static. */

		// Shortest and longest a feedback may be waited for (ms), and what is emitted
		// until there is a measurement to decide with.
		static constexpr int64_t MinSendIntervalMs{ 50 };
		static constexpr int64_t MaxSendIntervalMs{ 250 };
		static constexpr int64_t DefaultSendIntervalMs{ 100 };
		// What a feedback packet costs regardless of how many packets it reports on
		// (bytes): IPv4 20, UDP 8, SRTP 10 and an average report of 30.
		static constexpr int64_t FeedbackPacketOverhead{ 20 + 8 + 10 + 30 };
		// Share of the bitrate coming in that reporting it back is allowed to cost.
		static constexpr double FeedbackBitrateFraction{ 0.05 };
		// Window of the meter of incoming data. It measures what feedback is emitted
		// for, and a stream falling quiet for a moment must not lengthen the interval,
		// so it is long enough to only follow the sustained bitrate.
		static constexpr int64_t IncomingBitrateWindowMs{ 2500 };
		// How far back the arrival times are kept.
		static constexpr int64_t PacketArrivalTimestampWindowUs{ 500 * 1000 };

		/* Instance methods. */

		TransportCcFeedbackGenerator::TransportCcFeedbackGenerator(
		  Listener* listener, SharedInterface* shared, size_t maxRtcpPacketLen)
		  : maxRtcpPacketLen(maxRtcpPacketLen),
		    listener(listener),
		    shared(shared),
		    sendPeriodicTimer(shared->CreateTimer(this, "transport-cc-feedback-generator-send")),
		    incomingDataCounter(shared, /*ignorePaddingOnlyPackets*/ false, IncomingBitrateWindowMs),
		    sendIntervalMs(DefaultSendIntervalMs)
		{
			MS_TRACE();

			ResetFeedbackPacket(0);

			this->sendPeriodicTimer->Start(this->sendIntervalMs, this->sendIntervalMs);
		}

		void TransportCcFeedbackGenerator::IncomingPacket(int64_t arrivalTimeUs, const RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			uint16_t wideSeqNumber{ 0 };

			if (!packet->ReadTransportWideCc01(wideSeqNumber))
			{
				return;
			}

			// Only take note of the packet the first time it's received.
			if (!this->mapPacketArrivalTimes.try_emplace(wideSeqNumber, arrivalTimeUs).second)
			{
				return;
			}

			// A packet lower than the one the previous feedback started at may still
			// arrive, and it may have been reported as lost already, so the next
			// feedback has to start back there.
			if (
			  !this->feedbackWideSeqNumStart.has_value() ||
			  RTC::SeqManager<uint16_t>::IsSeqLowerThan(
			    wideSeqNumber, this->feedbackWideSeqNumStart.value()))
			{
				this->feedbackWideSeqNumStart = wideSeqNumber;
			}

			MayDropOldPacketArrivalTimes(wideSeqNumber, arrivalTimeUs);

			// The feedback reports on whoever sent last.
			this->feedbackMediaSsrc = packet->GetSsrc();

			this->incomingDataCounter.Update(packet);
		}

		void TransportCcFeedbackGenerator::FillAndSendFeedback()
		{
			MS_TRACE();

			if (!this->feedbackWideSeqNumStart.has_value())
			{
				return;
			}

			auto it = this->mapPacketArrivalTimes.lower_bound(this->feedbackWideSeqNumStart.value());

			// Everything known has already been reported, so there is nothing to
			// build a feedback packet out of.
			if (it == this->mapPacketArrivalTimes.end())
			{
				return;
			}

			while (it != this->mapPacketArrivalTimes.end())
			{
				const uint16_t sequenceNumber = it->first;
				const int64_t timestampUs     = it->second;
				// Whether nothing has been added to the current feedback packet yet,
				// which tells apart a packet that doesn't fit in what is left from one
				// that doesn't fit in an empty packet either.
				const bool feedbackPacketIsEmpty = !this->feedbackPacket->IsBaseSet();

				// If the base is not set in this packet let's set it.
				// NOTE: This may be needed many times during this loop, since the
				// current feedback packet may be a fresh new one if the previous one was
				// full (so already sent) or failed to be built.
				if (feedbackPacketIsEmpty)
				{
					// The base is where the report starts counting from, and every
					// sequence between it and the first one actually received is reported
					// as lost. Too long a run of those cannot be encoded, so the base is
					// moved up rather than letting the packet fail to be built.
					// NOTE: clang-tidy doesn't understand that this is guaranteed to have
					// a value: the method returns above when it has none, and nothing
					// takes it away afterwards, since SendFeedback() only ever assigns
					// one.
					// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
					uint16_t baseSequenceNumber = this->feedbackWideSeqNumStart.value();
					const uint16_t oldestReportableSequenceNumber =
					  sequenceNumber - RTC::RTCP::FeedbackRtpTransportPacket::maxMissingPackets;

					if (RTC::SeqManager<uint16_t>::IsSeqLowerThan(baseSequenceNumber, oldestReportableSequenceNumber))
					{
						baseSequenceNumber = oldestReportableSequenceNumber;
					}

					this->feedbackPacket->SetBase(baseSequenceNumber, timestampUs);
				}

				const auto result =
				  this->feedbackPacket->AddPacket(sequenceNumber, timestampUs, this->maxRtcpPacketLen);

				switch (result)
				{
					case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::SUCCESS:
					{
						// If the feedback packet is full, send it now.
						if (this->feedbackPacket->IsFull())
						{
							MS_DEBUG_DEV("feedback packet is full, sending it now");

							if (SendFeedback())
							{
								++this->feedbackPacketCount;
							}

							ResetFeedbackPacket(this->feedbackPacketCount);
						}

						++it;

						break;
					}

					case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::MAX_SIZE_EXCEEDED:
					{
						if (SendFeedback())
						{
							++this->feedbackPacketCount;
						}

						ResetFeedbackPacket(this->feedbackPacketCount);

						// NOTE: The iterator is only advanced when the packet had a chance
						// of fitting. If it didn't fit in a feedback packet that was
						// already empty it will not fit in the fresh one either, and
						// retrying it would loop forever.
						if (feedbackPacketIsEmpty)
						{
							++it;
						}

						break;
					}

					case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::FATAL:
					{
						// This packet cannot be expressed relative to the base and the
						// reference time of the feedback packet being filled, so what is
						// already in it goes out and a fresh one is started, whose base
						// and reference time are taken from this very packet.
						if (SendFeedback())
						{
							++this->feedbackPacketCount;
						}

						ResetFeedbackPacket(this->feedbackPacketCount);

						// NOTE: Same reasoning as above. If it didn't fit in a feedback
						// packet that was already empty, a fresh one changes nothing.
						if (feedbackPacketIsEmpty)
						{
							++it;
						}

						break;
					}
				}
			}

			// The packet may hold nothing, but then SendFeedback() won't send it.
			if (SendFeedback())
			{
				++this->feedbackPacketCount;
			}

			ResetFeedbackPacket(this->feedbackPacketCount);
		}

		bool TransportCcFeedbackGenerator::SendFeedback()
		{
			MS_TRACE();

			this->feedbackPacket->Finish();

			if (!this->feedbackPacket->IsSerializable())
			{
				MS_WARN_TAG(rtcp, "couldn't send feedback-cc packet because it is not serializable");

				return false;
			}

			// The report is about whoever has sent last, which this packet could not
			// be built with: it was started at the end of the previous round, when
			// that may not have been known yet.
			this->feedbackPacket->SetMediaSsrc(this->feedbackMediaSsrc);

			const auto latestWideSeqNumber = this->feedbackPacket->GetLatestSequenceNumber();

			this->listener->OnTransportCcFeedbackGeneratorSendRtcpPacket(this, this->feedbackPacket.get());

			this->feedbackWideSeqNumStart = latestWideSeqNumber + 1;

			return true;
		}

		void TransportCcFeedbackGenerator::MayDropOldPacketArrivalTimes(uint16_t seqNum, int64_t arrivalTimeUs)
		{
			MS_TRACE();

			// Ignore the given instant if it's smaller than the window in order to
			// avoid negative values (should never happen) and return early if the
			// condition is met.
			if (arrivalTimeUs < PacketArrivalTimestampWindowUs)
			{
				return;
			}

			if (!this->feedbackWideSeqNumStart.has_value())
			{
				return;
			}

			const uint16_t feedbackWideSeqNumStart = this->feedbackWideSeqNumStart.value();
			const int64_t expiryTimestampUs        = arrivalTimeUs - PacketArrivalTimestampWindowUs;
			auto it                                = this->mapPacketArrivalTimes.begin();

			while (it != this->mapPacketArrivalTimes.end() && it->first != feedbackWideSeqNumStart &&
			       RTC::SeqManager<uint16_t>::IsSeqLowerThan(it->first, seqNum) &&
			       it->second <= expiryTimestampUs)
			{
				it = this->mapPacketArrivalTimes.erase(it);
			}
		}

		void TransportCcFeedbackGenerator::ResetFeedbackPacket(uint8_t feedbackPacketCount)
		{
			MS_TRACE();

			// NOTE: The media SSRC is left at zero here and filled in when the packet
			// is about to go out, since who has sent last may still change while this
			// one is being filled.
			this->feedbackPacket = std::make_unique<RTC::RTCP::FeedbackRtpTransportPacket>(
			  /*senderSsrc*/ 0, /*mediaSsrc*/ 0);

			this->feedbackPacket->SetFeedbackPacketCount(feedbackPacketCount);
		}

		int64_t TransportCcFeedbackGenerator::ComputeSendIntervalMs(int64_t incomingBitrate)
		{
			MS_TRACE();

			// Bitrate the feedback is allowed to take, which is what decides how long
			// its fixed cost may be spread over.
			const double feedbackBitrate = static_cast<double>(incomingBitrate) * FeedbackBitrateFraction;
			// NOTE: Checked against the bitrate rather than against the resulting
			// interval, so that a bitrate of zero is not divided by.
			const double minFeedbackBitrate = (static_cast<double>(FeedbackPacketOverhead) * 8 * 1000) /
			                                  static_cast<double>(MaxSendIntervalMs);

			if (feedbackBitrate <= minFeedbackBitrate)
			{
				return MaxSendIntervalMs;
			}

			const auto intervalMs = static_cast<int64_t>(
			  (static_cast<double>(FeedbackPacketOverhead) * 8 * 1000) / feedbackBitrate);

			return std::max(intervalMs, MinSendIntervalMs);
		}

		void TransportCcFeedbackGenerator::MayUpdateSendInterval(int64_t nowMs)
		{
			MS_TRACE();

			const auto incomingBitrate = this->incomingDataCounter.GetBitrate(nowMs);

			if (!incomingBitrate.has_value())
			{
				return;
			}

			const int64_t sendIntervalMs = ComputeSendIntervalMs(incomingBitrate.value());

			if (sendIntervalMs == this->sendIntervalMs)
			{
				return;
			}

			MS_DEBUG_DEV(
			  "send interval changed [old:%" PRIi64 " ms, new:%" PRIi64 " ms, incomingBitrate:%" PRIi64
			  " bps]",
			  this->sendIntervalMs,
			  sendIntervalMs,
			  incomingBitrate.value());

			this->sendIntervalMs = sendIntervalMs;

			this->sendPeriodicTimer->Restart(this->sendIntervalMs, this->sendIntervalMs);
		}

		void TransportCcFeedbackGenerator::OnTimer(TimerHandleInterface* timer)
		{
			MS_TRACE();

			if (timer == this->sendPeriodicTimer.get())
			{
				FillAndSendFeedback();

				MayUpdateSendInterval(this->shared->GetTimeMs());
			}
		}
	} // namespace BWE
} // namespace RTC
