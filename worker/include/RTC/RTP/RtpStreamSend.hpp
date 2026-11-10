#ifndef MS_RTC_RTP_RTP_STREAM_SEND_HPP
#define MS_RTC_RTP_RTP_STREAM_SEND_HPP

#include "common.hpp"
#include "FBS/rtpStream.h"
#include "Utils/UnwrappedSequenceNumber.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/RTCP/packet/AnyPayloadSpecificFeedbackPacket.hpp"
#include "RTC/RTCP/packet/AnyReportPacket.hpp"
#include "RTC/RTCP/packet/CompoundPacket.hpp"
#include "RTC/RTCP/packet/NackPacket.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RetransmissionBuffer.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RateCalculator.hpp"
#include "SharedInterface.hpp"
#include "Utils.hpp"
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
				 * Packets lost, held at zero and at the packets expected. A hole that a
				 * previous report already counted and that a late packet has filled in
				 * since may well be one the uplink of the sender lost and that was never
				 * charged to this link, so giving it back would tell about a path other
				 * than this one.
				 */
				int64_t lostPackets;
				/**
				 * Packets expected, which is what the sequence numbers of the interval
				 * covered once the ones that were never sent are taken out of it.
				 */
				int64_t expectedPackets;
			};

		public:
			/**
			 * What a Receiver Report says about the link, each part missing when the
			 * report carries nothing to work it out with.
			 */
			struct ReceiverReportResult
			{
				/**
				 * How many packets the remote endpoint missed and how many it should
				 * have got, or no value if this report measures no interval at all.
				 */
				std::optional<Loss> loss;
				/**
				 * Round trip time just measured (ms), or no value if this report refers
				 * to no Sender Report to measure it against.
				 */
				std::optional<float> rttMs;
			};

		public:
			struct ReceivePacketResult
			{
				/**
				 * Whether the packet was accepted by the stream.
				 */
				bool accepted{ false };
				/**
				 * The retransmission buffer item the packet info was stored into, or
				 * nullptr if it was not stored (because the packet was discarded or
				 * because NACK is not enabled in this stream).
				 *
				 * @remarks
				 * - Its `sharedPacket` is still empty and must be filled by the caller
				 *   once the packet has been restored to its original state.
				 * - The item is owned by the retransmission buffer and is only valid
				 *   until the next call that may evict items from it, so the caller
				 *   must fill it right away.
				 */
				RTP::RetransmissionBuffer::Item* retransmissionBufferItem{ nullptr };
			};

		public:
			class Listener : public RTP::RtpStream::Listener
			{
			public:
				virtual void OnRtpStreamRetransmitRtpPacket(
				  RTP::RtpStreamSend* rtpStream, RTP::Packet* packet) = 0;
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

			ReceivePacketResult ReceivePacket(RTP::Packet* packet);

			void ReceiveRtcpNackPacket(const RTC::RTCP::NackPacket* nackPacket);

			void ReceiveRtcpKeyFrameRequest(
			  RTC::RTCP::AnyPayloadSpecificFeedbackPacket::MessageType messageType);

			/**
			 * @returns What this report block says about the link.
			 */
			ReceiverReportResult ReceiveRtcpReportBlock(
			  const RTC::RTCP::AnyReportPacket::ReportBlock& reportBlock, int64_t receivedAtUs);

			/**
			 * Whether this stream has something recent enough to tell in a Sender
			 * Report, which is what `AddRtcpSenderReportPacket()` requires.
			 */
			[[nodiscard]] bool HasRtcpSenderReport(int64_t nowUs) const
			{
				if (this->transmissionCounter.GetPacketCount() == 0)
				{
					return false;
				}

				// A stream that stopped sending cannot tell where its RTP timeline is
				// now, and extrapolating would claim RTP timestamps of packets never
				// sent.
				return Utils::Time::TimeUsToMs(nowUs - this->maxPacketAtUs) <=
				       RtpStreamSend::MaxSenderReportReferenceAgeMs;
			}

			/**
			 * Add a Sender Report packet about this stream to the given compound
			 * packet.
			 *
			 * @returns Whether it was added, which does not happen when there is
			 *   nothing recent enough to report or when it does not fit in the
			 *   compound packet. In both cases the compound packet is left untouched.
			 */
			[[nodiscard]] bool AddRtcpSenderReportPacket(
			  RTC::RTCP::CompoundPacket* compoundPacket, int64_t nowUs);

			/**
			 * Add a SDES chunk with the CNAME of this stream to the given SDES packet.
			 *
			 * @returns Whether it was added, which does not happen when the packet
			 *   already has the most chunks it can count or when it does not fit. In
			 *   both cases the SDES packet is left untouched.
			 */
			[[nodiscard]] bool AddRtcpSdesChunk(RTC::RTCP::SdesPacket* sdesPacket) const;

			/**
			 * Length (in bytes) of the SDES chunk `AddRtcpSdesChunk()` adds.
			 */
			size_t GetRtcpSdesChunkLength() const;

			void Pause() override;

			void Resume() override;

			/**
			 * Bitrate of everything that has been sent.
			 */
			int64_t GetBitrate(int64_t nowMs) override
			{
				return this->transmissionCounter.GetBitrate(nowMs).value_or(0);
			}

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
			 * Determine how much of what the Receiver Report reports as lost really
			 * was lost on the way to the remote endpoint, and take note of it.
			 *
			 * @returns What this link lost and what it was expected to deliver since
			 *   the previous Receiver Report, or no value if this one measures no
			 *   interval at all.
			 *
			 * @remarks
			 * - What it returns, and what it takes note of in `fractionLost` and
			 *   `packetsLost`, is held at zero and at the packets expected, since none
			 *   of them may go backwards.
			 */
			std::optional<Loss> UpdateSendLoss(const RTC::RTCP::AnyReportPacket::ReportBlock& reportBlock);

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
			const std::string mid;
			uint16_t rtxSeq{ 0 };
			RTC::RtpDataCounter transmissionCounter;
			// Payload bytes of everything sent, leaving out RTP headers and padding,
			// which is what the octet count of a Sender Report tells.
			uint64_t sentPayloadBytes{ 0 };
			std::unique_ptr<RTP::RetransmissionBuffer> retransmissionBuffer;
			// Sequence numbers requested by received NACKs and not retransmitted yet,
			// in the order they were requested.
			std::deque<uint16_t> pendingRetransmissionsQueue;
			// Same sequence numbers as in pendingRetransmissionsQueue, to tell whether
			// a requested one is already pending.
			ankerl::unordered_dense::set<uint16_t> pendingRetransmissionsSet;
			// Timer to retransmit the pending retransmissions spaced in time. Only
			// created if NACK is enabled.
			const std::unique_ptr<TimerHandleInterface> retransmissionTimer;
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
