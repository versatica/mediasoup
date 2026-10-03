#ifndef MS_RTC_RTP_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_RTP_STREAM_SEND_HPP

#include "common.hpp"
#include "FBS/rtpStream.h"
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

			void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report, int64_t receivedAtUs);

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

			void UpdateScore(RTC::RTCP::ReceiverReport* report);

			/* Pure virtual methods inherited from RTP::RtpStream. */
		public:
			void UserOnSequenceNumberReset() override;

			/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
		public:
			void OnTimer(TimerHandleInterface* timer) override;

		private:
			// Packets lost at last interval for score calculation.
			int32_t lostPriorScore{ 0 };
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
		};
	} // namespace RTP
} // namespace RTC

#endif
