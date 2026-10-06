#ifndef MS_RTC_RTP_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_RTP_STREAM_SEND_HPP

#include "common.hpp"
#include "FBS/rtpStream.h"
#include "Utils/UnwrappedSequenceNumber.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTCP/XrReceiverReferenceTime.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RetransmissionBuffer.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "RTC/RateCalculator.hpp"
#include "SharedInterface.hpp"
#include <flatbuffers/flatbuffer_builder.h>
#include <ankerl/unordered_dense.h>
#include <deque>
#include <set>
#include <string>

namespace RTC
{
	namespace RTP
	{
		class RtpStreamSend : public RTP::RtpStream, public TimerHandleInterface::Listener
		{
		public:
			/**
			 * Maximum retransmission buffer size for video (ms).
			 */
			static constexpr int64_t MaxRetransmissionDelayForVideoMs{ 4000 };
			/**
			 * Maximum retransmission buffer size for audio (ms).
			 */
			static constexpr int64_t MaxRetransmissionDelayForAudioMs{ 2000 };
			/**
			 * How old the last packet sent may be for a Sender Report to still be generated
			 * (ms).
			 *
			 * @remarks
			 * - Above the interval between packets of any legitimate stream, including Opus
			 *   with a 120 ms ptime and screen sharing at 1 fps, so that it only triggers on
			 *   a stream that has really stopped sending.
			 */
			static constexpr int64_t MaxSenderReportReferenceAgeMs{ 2000 };
			/**
			 * How many sequence numbers that were never sent are remembered.
			 *
			 * @remarks
			 * - Only a Receiver Report drains them and a remote endpoint may stop
			 *   reporting, so this bounds what a stream whose reports never arrive can
			 *   hold on to.
			 * - What it has to cover is the sequence numbers skipped between two
			 *   Receiver Reports. Taking the 5 seconds that RFC 3550 gives as a report
			 *   interval and the 520 packets per second of a 5 Mbps video stream, this
			 *   is close to four seconds of such a stream skipped whole.
			 */
			static constexpr size_t MaxUnsentSeqNumbers{ 2000 };

		public:
			/**
			 * How many packets the remote endpoint missed and how many it should have
			 * got, over the time between the previous Receiver Report and this one,
			 * counting only what this link is answerable for.
			 */
			struct Loss
			{
				/**
				 * Packets lost. It may be negative, since a hole that a previous report
				 * already counted and that a late packet has filled in since gives back
				 * what that report was charged for.
				 */
				int64_t lostPackets;
				/**
				 * Packets expected, which is what the sequence numbers of the interval
				 * covered once the ones that were never sent are taken out of it.
				 */
				int64_t expectedPackets;
			};

		public:
			enum class ReceivePacketResult : uint8_t
			{
				DISCARDED,
				ACCEPTED_AND_NOT_STORED,
				ACCEPTED_AND_STORED
			};

		public:
			class Listener : public RTP::RtpStream::Listener
			{
			public:
				virtual void OnRtpStreamRetransmitRtpPacket(
				  RTP::RtpStreamSend* rtpStream, RTP::Packet* packet) = 0;
			};

		private:
			/**
			 * Data of a received Receiver Reference Time Extended Report needed to
			 * report LRR and DLRR back in a Delay Since Last Receiver Report Extended
			 * Report.
			 */
			struct ReceiverReferenceTime
			{
				/**
				 * Middle 32 bits out of 64 in the NTP timestamp of the Receiver Reference Time.
				 */
				uint32_t compactNtp;
				/**
				 * Local time at which the Receiver Reference Time arrived.
				 */
				int64_t receivedAtUs;
			};

		public:
			RtpStreamSend(
			  RTP::RtpStreamSend::Listener* listener,
			  SharedInterface* shared,
			  RTP::RtpStream::Params& params,
			  std::string& mid);

			~RtpStreamSend() override;

		public:
			flatbuffers::Offset<FBS::RtpStream::Stats> FillBufferStats(
			  flatbuffers::FlatBufferBuilder& builder) override;

			void SetRtx(uint8_t payloadType, uint32_t ssrc) override;

			ReceivePacketResult ReceivePacket(RTP::Packet* packet, const RTP::SharedPacket& sharedPacket);

			void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);

			void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType);

			/**
			 * @returns What this link lost and what it was expected to deliver since
			 *   the previous Receiver Report, or no value if this one measures no
			 *   interval at all.
			 */
			std::optional<Loss> ReceiveRtcpReceiverReport(
			  RTC::RTCP::ReceiverReport* report, int64_t receivedAtUs);

			void ReceiveRtcpXrReceiverReferenceTime(
			  RTC::RTCP::ReceiverReferenceTime* report, int64_t receivedAtUs);

			RTC::RTCP::SenderReport* GetRtcpSenderReport(int64_t nowUs);

			RTC::RTCP::DelaySinceLastRr::SsrcInfo* GetRtcpXrDelaySinceLastRrSsrcInfo(int64_t nowUs);

			RTC::RTCP::SdesChunk* GetRtcpSdesChunk();

			void Pause() override;

			void Resume() override;

			int64_t GetBitrate(int64_t nowMs) override
			{
				return this->transmissionCounter.GetBitrate(nowMs).value_or(0);
			}

			int64_t GetBitrate(int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;

			int64_t GetSpatialLayerBitrate(int64_t nowMs, uint8_t spatialLayer) override;

			int64_t GetLayerBitrate(int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;

		private:
			void RetransmitPendingPackets();

			/**
			 * @param loss What the latest Receiver Report measured, or no value if it
			 *   measured nothing.
			 */
			void UpdateScore(const std::optional<Loss>& loss);

			/**
			 * Take note of the sequence numbers that the packet just accepted leaves
			 * behind for good, or forget one that it fills in late.
			 */
			void UpdateUnsentSeqNumbers(uint16_t seq);

			/**
			 * Work out how much of what the Receiver Report reports as lost really was
			 * lost on the way to the remote endpoint, and take note of it.
			 *
			 * @returns What this link lost and what it was expected to deliver since
			 *   the previous Receiver Report, or no value if this one measures no
			 *   interval at all.
			 *
			 * @remarks
			 * - What it takes note of in `fractionLost` and `packetsLost` is held at
			 *   zero and at the packets expected, since neither may go backwards. What
			 *   it returns is not, so that whoever adds up several of them gets the
			 *   negative ones back.
			 */
			std::optional<Loss> UpdateSendLoss(RTC::RTCP::ReceiverReport* report);

			/**
			 * Forget everything measured over a numbering that is not in use anymore.
			 */
			void ResetSendLoss();

			/* Pure virtual methods inherited from RTP::RtpStream. */
		public:
			void UserOnSequenceNumberReset() override;

			/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
		public:
			void OnTimer(TimerHandleInterface* timer) override;

		private:
			// Packets sent at last interval for score calculation.
			// NOTE: As wide as the counter it snapshots, which does not wrap, so that
			// the difference against it stays exact however long the stream runs.
			uint64_t sentPriorScore{ 0 };
			std::string mid;
			uint16_t rtxSeq{ 0 };
			RTC::RtpDataCounter transmissionCounter;
			RTP::RetransmissionBuffer* retransmissionBuffer{ nullptr };
			// Sequence numbers requested by received NACKs and not retransmitted yet,
			// in the order they were requested.
			std::deque<uint16_t> pendingRetransmissionsQueue;
			// Same sequence numbers as in pendingRetransmissionsQueue, to tell whether
			// a requested one is already pending.
			ankerl::unordered_dense::set<uint16_t> pendingRetransmissionsSet;
			// Timer to retransmit the pending retransmissions spaced in time. Only
			// created if NACK is enabled.
			const std::unique_ptr<TimerHandleInterface> retransmissionTimer;
			// Timing data of the most recent Receiver Reference Time received.
			std::optional<ReceiverReferenceTime> lastReceiverReferenceTime;
			// Unwraps the sequence numbers of what is sent, so that comparing them
			// against the ones a Receiver Report names needs no wrapping arithmetic.
			Utils::UnwrappedSequenceNumber<uint16_t>::Unwrapper seqUnwrapper;
			// Sequence numbers that were skipped and will never be sent, which is what
			// the uplink of the Producer lost, unwrapped.
			std::set<int64_t> unsentSeqs;
			// Highest sequence number sent so far, unwrapped.
			std::optional<int64_t> highestSentSeq;
			// Highest sequence number the previous Receiver Report had seen, unwrapped
			// into our own numbering, which is where the interval of the next one
			// starts. Seeded one below the first sequence number sent, so that the
			// first report measures from that one onwards.
			std::optional<int64_t> lastRrSeq;
			// What the previous Receiver Report reported as lost in total. Left without
			// a value when the numbering is reset, since the remote endpoint counts
			// over the whole session and does not reset along with us, so the next
			// report can only be taken as a new mark.
			std::optional<int32_t> lastRrTotalLost{ 0 };
		};
	} // namespace RTP
} // namespace RTC

#endif
