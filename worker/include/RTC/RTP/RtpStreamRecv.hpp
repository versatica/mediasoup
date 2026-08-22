#ifndef MS_RTC_RTP_RTP_STREAM_RECV_HPP
#define MS_RTC_RTP_RTP_STREAM_RECV_HPP

#include "handles/TimerHandleInterface.hpp"
#include "RTC/NackGenerator.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RateCalculator.hpp"
#include <vector>

namespace RTC
{
	namespace RTP
	{
		class RtpStreamRecv : public RTP::RtpStream,
		                      public RTC::NackGenerator::Listener,
		                      public TimerHandleInterface::Listener
		{
		public:
			/**
			 * How far the RTP timestamp of a packet may be from the last received
			 * `abs-capture-time` RTP header extension for its capture instant to still be
			 * interpolated from it (ms).
			 *
			 * @remarks
			 * - Same value libwebrtc uses in `AbsoluteCaptureTimeInterpolator`, which
			 *   pairs with senders emitting the extension at least once per second.
			 */
			static constexpr uint64_t MaxAbsCaptureTimeInterpolationMs{ 5000 };
			/**
			 * How far the RTP timestamp of a packet may be from the last received RTCP
			 * Sender Report for its capture instant to still be interpolated from it (ms).
			 */
			static constexpr uint64_t MaxSenderReportInterpolationMs{ 10000 };

		public:
			class Listener : public RTP::RtpStream::Listener
			{
			public:
				virtual void OnRtpStreamSendRtcpPacket(
				  RTP::RtpStreamRecv* rtpStream, RTC::RTCP::Packet* packet) = 0;
				/**
				 * Obtains the worst remote fraction lost of all the RtpStreamSend
				 * streams consuming this RtpStreamRecv.
				 */
				virtual uint8_t OnRtpStreamNeedWorstRemoteFractionLost(RTP::RtpStreamRecv* rtpStream) = 0;
			};

		public:
			class TransmissionCounter
			{
			public:
				TransmissionCounter(
				  SharedInterface* shared, uint8_t spatialLayers, uint8_t temporalLayers, size_t windowSize);

			public:
				void Update(const RTP::Packet* packet);

				uint32_t GetBitrate(uint64_t nowMs);

				uint32_t GetBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer);

				uint32_t GetSpatialLayerBitrate(uint64_t nowMs, uint8_t spatialLayer);

				uint32_t GetLayerBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer);

				size_t GetPacketCount() const;

				size_t GetBytes() const;

			private:
				std::vector<std::vector<RTC::RtpDataCounter>> spatialLayerCounters;
			};

		public:
			/**
			 * Correspondence between the RTP timeline of this stream and our own monotonic
			 * clock.
			 */
			struct CaptureMapping
			{
				/**
				 * Capture instant of the media carried by the RTP timestamp below, in our own
				 * monotonic clock (ms).
				 */
				uint64_t captureMs;
				/**
				 * RTP timestamp the capture instant above refers to.
				 */
				uint32_t ts;
			};

		private:
			/**
			 * Data of a received RTCP Sender Report needed to report LSR and DLSR back in a
			 * Receiver Report.
			 */
			struct SenderReportTiming
			{
				/**
				 * Middle 32 bits out of 64 in the NTP timestamp of the Sender Report.
				 */
				uint32_t compactNtp;
				/**
				 * Local time at which the Sender Report arrived.
				 */
				uint64_t receivedMs;
			};

			/**
			 * Capture instant carried by an `abs-capture-time` RTP header extension, together
			 * with the RTP timestamp it refers to.
			 */
			struct AbsCaptureTime
			{
				/**
				 * RTP timestamp the capture instant below refers to.
				 */
				uint32_t ts;
				/**
				 * Capture instant in the remote sender's wall clock (ms).
				 */
				uint64_t ntpMs;
			};

		public:
			RtpStreamRecv(
			  RTP::RtpStreamRecv::Listener* listener,
			  SharedInterface* shared,
			  RTP::RtpStream::Params& params,
			  uint32_t sendNackDelayMs,
			  bool useRtpInactivityCheck);

			~RtpStreamRecv() override;

		public:
			flatbuffers::Offset<FBS::RtpStream::Stats> FillBufferStats(
			  flatbuffers::FlatBufferBuilder& builder) override;

			bool ReceivePacket(RTP::Packet* packet);

			bool ReceiveRtxPacket(RTP::Packet* packet);

			RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();

			RTC::RTCP::ReceiverReport* GetRtxRtcpReceiverReport();

			void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);

			void ReceiveRtxRtcpSenderReport(RTC::RTCP::SenderReport* report);

			void ReceiveRtcpXrDelaySinceLastRr(RTC::RTCP::DelaySinceLastRr::SsrcInfo* ssrcInfo);

			/**
			 * Local time at which the last RTCP Sender Report arrived.
			 *
			 * @returns No value if no Sender Report has arrived yet.
			 */
			std::optional<uint64_t> GetSenderReportReceivedMs() const
			{
				if (!this->lastSenderReportTiming.has_value())
				{
					return std::nullopt;
				}

				return this->lastSenderReportTiming.value().receivedMs;
			}

			/**
			 * Store the correspondence between the RTP timeline of this stream and our own
			 * monotonic clock.
			 *
			 * @remarks
			 * - Only the Producer can tell it, since translating the capture instant of the
			 *   remote sender into our own clock needs the whole set of streams of that
			 *   sender.
			 *
			 * @param captureMs - Capture instant of `ts` in our own monotonic clock.
			 * @param ts - RTP timestamp the capture instant refers to.
			 */
			void SetCaptureMapping(uint64_t captureMs, uint32_t ts)
			{
				this->lastCaptureMapping = RTP::RtpStreamRecv::CaptureMapping{
					.captureMs = captureMs,
					.ts        = ts,
				};
			}

			/**
			 * Correspondence between the RTP timeline of this stream and our own monotonic
			 * clock, as of the last RTP timestamp whose capture instant could be told.
			 *
			 * @remarks
			 * - Once set it is never unset, so any two streams of a same sender that have it
			 *   can always be aligned to each other.
			 *
			 * @returns No value if no capture instant could be told yet.
			 */
			std::optional<RTP::RtpStreamRecv::CaptureMapping> GetCaptureMapping() const
			{
				return this->lastCaptureMapping;
			}

			/**
			 * Capture instant of the given RTP timestamp, expressed in the remote sender's
			 * wall clock, interpolated from the last received `abs-capture-time` RTP header
			 * extension.
			 *
			 * @param ts - RTP timestamp whose capture instant is wanted.
			 *
			 * @returns No value if no such extension has been received, or if the given RTP
			 * timestamp is too far away from the one it referred to.
			 */
			std::optional<uint64_t> GetRemoteCaptureMsFromAbsCaptureTime(uint32_t ts) const;

			/**
			 * Capture instant of the given RTP timestamp, expressed in the remote sender's
			 * wall clock, interpolated from the last received RTCP Sender Report.
			 *
			 * @param ts - RTP timestamp whose capture instant is wanted.
			 *
			 * @returns No value if no Sender Report has been received, or if the given RTP
			 * timestamp is too far away from the one it reported.
			 */
			std::optional<uint64_t> GetRemoteCaptureMsFromSenderReport(uint32_t ts) const;

			void RequestKeyFrame();

			void Pause() override;

			void Resume() override;

			uint32_t GetBitrate(uint64_t nowMs) override
			{
				return this->transmissionCounter.GetBitrate(nowMs);
			}

			uint32_t GetBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override
			{
				return this->transmissionCounter.GetBitrate(nowMs, spatialLayer, temporalLayer);
			}

			uint32_t GetSpatialLayerBitrate(uint64_t nowMs, uint8_t spatialLayer) override
			{
				return this->transmissionCounter.GetSpatialLayerBitrate(nowMs, spatialLayer);
			}

			uint32_t GetLayerBitrate(uint64_t nowMs, uint8_t spatialLayer, uint8_t temporalLayer) override
			{
				return this->transmissionCounter.GetLayerBitrate(nowMs, spatialLayer, temporalLayer);
			}

			bool HasRtpInactivityCheckEnabled() const
			{
				return this->useRtpInactivityCheck;
			}

		private:
			void CalculateJitter(uint32_t rtpTimestamp);

			void UpdateScore();

			/**
			 * Interpolate the capture instant of `ts` from a reference pair, all of them
			 * expressed in the remote sender's wall clock.
			 *
			 * @param referenceNtpMs - Capture instant of `referenceTs`.
			 * @param referenceTs - RTP timestamp the reference instant refers to.
			 * @param ts - RTP timestamp whose capture instant is wanted.
			 * @param maxDistanceMs - How far `ts` may be from `referenceTs`.
			 */
			std::optional<uint64_t> InterpolateRemoteCaptureMs(
			  uint64_t referenceNtpMs, uint32_t referenceTs, uint32_t ts, uint64_t maxDistanceMs) const;

			/* Pure virtual methods inherited from RTP::RtpStream. */
		public:
			void UserOnSequenceNumberReset() override;

			/* Pure virtual methods inherited from TimerHandleInterface. */
		protected:
			void OnTimer(TimerHandleInterface* timer) override;

			/* Pure virtual methods inherited from RTC::NackGenerator. */
		protected:
			void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) override;

			void OnNackGeneratorKeyFrameRequired() override;

		private:
			// Passed by argument.
			uint32_t sendNackDelayMs{ 0u };
			bool useRtpInactivityCheck{ false };
			// Others.
			// Packets expected at last interval.
			uint32_t expectedPrior{ 0u };
			// Packets expected at last interval for score calculation.
			uint32_t expectedPriorScore{ 0u };
			// Packets received at last interval.
			uint32_t receivedPrior{ 0u };
			// Packets received at last interval for score calculation.
			uint32_t receivedPriorScore{ 0u };
			// Timing data of the most recent Sender Report received.
			std::optional<SenderReportTiming> lastSenderReportTiming;
			// Most recent `abs-capture-time` RTP header extension received.
			std::optional<AbsCaptureTime> lastAbsCaptureTime;
			// Most recent RTP timestamp whose capture instant could be told, along with it.
			std::optional<CaptureMapping> lastCaptureMapping;
			// Relative transit time for prev packet.
			int32_t transit{ 0u };
			uint8_t firSeqNumber{ 0u };
			int32_t reportedPacketsLost{ 0 };
			std::unique_ptr<RTC::NackGenerator> nackGenerator;
			TimerHandleInterface* inactivityCheckPeriodicTimer{ nullptr };
			bool inactive{ false };
			// Valid media + valid RTX.
			TransmissionCounter transmissionCounter;
			// Just valid media.
			RTC::RtpDataCounter mediaTransmissionCounter;
			// Template dependency structure for Dependency Descriptor.
			std::unique_ptr<RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
			  templateDependencyStructure;
		};
	} // namespace RTP
} // namespace RTC

#endif
