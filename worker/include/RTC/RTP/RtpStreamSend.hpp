#ifndef MS_RTC_RTP_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_RTP_STREAM_SEND_HPP

#include "RTC/RTP/RetransmissionBuffer.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/SharedPacket.hpp"
#include "RTC/RateCalculator.hpp"

namespace RTC
{
	namespace RTP
	{
		class RtpStreamSend : public RTP::RtpStream
		{
		public:
			/**
			 * Maximum retransmission buffer size for video (ms).
			 */
			static constexpr int64_t MaxRetransmissionDelayForVideoMs{ 2000 };
			/**
			 * Maximum retransmission buffer size for audio (ms).
			 */
			static constexpr int64_t MaxRetransmissionDelayForAudioMs{ 1000 };
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
				return this->transmissionCounter.GetBitrate(nowMs);
			}

			int64_t GetBitrate(int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;

			int64_t GetSpatialLayerBitrate(int64_t nowMs, uint8_t spatialLayer) override;

			int64_t GetLayerBitrate(int64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override;

		private:
			void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask);

			void UpdateScore(RTC::RTCP::ReceiverReport* report);

			/* Pure virtual methods inherited from RTP::RtpStream. */
		public:
			void UserOnSequenceNumberReset() override;

		private:
			// Packets lost at last interval for score calculation.
			int32_t lostPriorScore{ 0 };
			// Packets sent at last interval for score calculation.
			uint32_t sentPriorScore{ 0 };
			std::string mid;
			uint16_t rtxSeq{ 0 };
			RTC::RtpDataCounter transmissionCounter;
			RTP::RetransmissionBuffer* retransmissionBuffer{ nullptr };
			// Timing data of the most recent Receiver Reference Time received.
			std::optional<ReceiverReferenceTime> lastReceiverReferenceTime;
		};
	} // namespace RTP
} // namespace RTC

#endif
