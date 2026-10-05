#ifndef MS_RTC_BWE_SENDER_TRANSPORT_CONGESTION_CONTROLLER_HPP
#define MS_RTC_BWE_SENDER_TRANSPORT_CONGESTION_CONTROLLER_HPP

#include "common.hpp"
#include "handles/TimerHandle.hpp"
#include "RTC/BWE/AlrDetector.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/DelayBasedBwe.hpp"
#include "RTC/BWE/FeedbackAdapter.hpp"
#include "RTC/BWE/PacketLossTracker.hpp"
#include "RTC/BWE/ProbeBitrateEstimator.hpp"
#include "RTC/BWE/ProbeController.hpp"
#include "RTC/BWE/ProbingScheduler.hpp"
#include "RTC/BWE/RobustThroughputEstimator.hpp"
#include "RTC/BWE/SendPacketHistory.hpp"
#include "RTC/BWE/TargetRateController.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTP/Packet.hpp"
#include "SharedInterface.hpp"
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Everything the sending side of a transport does about congestion, which
		 * amounts to deciding how much may be sent towards the remote endpoint.
		 *
		 * It owns the whole control loop and is, along with its receiving
		 * counterpart, the only part of the module that the transport deals with.
		 * What comes in is what leaves this endpoint and what the remote one reports
		 * back about it; what comes out is a target bitrate, for whoever shares it
		 * among the streams being sent, and the packets of a probe when the link has
		 * to be measured beyond what the media alone can tell.
		 */
		class SenderTransportCongestionController : public ProbingScheduler::Listener,
		                                            public TimerHandleInterface::Listener
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * The bitrate that may be sent towards the remote endpoint changed
				 * (bps).
				 */
				virtual void OnSenderTransportCongestionControllerTargetBitrate(
				  SenderTransportCongestionController* senderTransportCongestionController,
				  int64_t targetBitrate) = 0;

				/**
				 * A packet of a probe is ready to go out.
				 *
				 * @param sequenceNumber - The one given to the packet, which has to be
				 *   given back through `OnRtpPacketSent()` once the packet has actually
				 *   left, just like for a packet of media.
				 *
				 * @returns Whether the rest of the probe is still wanted, so that a
				 *   packet that couldn't be sent stops the ones behind it.
				 *
				 * @remarks
				 * - The packet must be sent before returning, since the next one is
				 *   built over the very same instance.
				 */
				virtual bool OnSenderTransportCongestionControllerSendRtpPacket(
				  SenderTransportCongestionController* senderTransportCongestionController,
				  RTC::RTP::Packet* packet,
				  int64_t sequenceNumber) = 0;
			};

			/**
			 * What a packet about to be sent cannot tell about itself.
			 */
			struct RtpPacketToBeSentOptions
			{
				/**
				 * Whether it belongs to an audio stream.
				 */
				bool isAudio{ false };
				/**
				 * Whether it is being sent again after having been reported lost.
				 */
				bool isRetransmission{ false };
				/**
				 * SSRC of the stream it retransmits, when it goes out over RTX.
				 */
				std::optional<uint32_t> originalSsrc;
			};

			struct SenderTransportCongestionControllerOptions
			{
				/**
				 * Bitrate the control loop starts from, before anything has been
				 * measured (bps). It only means something until the first feedback
				 * arrives, which is why it is given once and never changed: starting
				 * over would mean throwing away what has been learnt since.
				 */
				int64_t startBitrate{ 300000 };
				/**
				 * Least that may ever be estimated (bps). It exists so that the loop
				 * cannot drive its own estimate to nothing, which would be
				 * unrecoverable: with nothing to hand out, nothing is sent, so there is
				 * nothing to measure and no way back up.
				 */
				int64_t minBitrate{ RTC::Consts::BweMinBitrate };
				/**
				 * Most that may ever be estimated (bps), or `Types::BitrateInfinite` to
				 * let the link decide.
				 */
				int64_t maxBitrate{ Types::BitrateInfinite };
			};

		private:
			/**
			 * What a call leaves to be done once it is over, so that nothing of this
			 * class is half way through anything by the time the listener is told and
			 * calls back in.
			 */
			struct PendingUpdate
			{
				/**
				 * Bursts that were asked for, which are handed to the scheduler all at
				 * once.
				 */
				std::vector<Types::ProbeClusterConfig> probeClusterConfigs;
				/**
				 * Target the listener is to be told about, when something it reasons
				 * about turned out to have changed.
				 *
				 * @remarks
				 * - It is worked out where the call would have announced it, and only
				 *   handed over at the end, since what is reconsidered along the way
				 *   depends on the order the original does things in.
				 */
				std::optional<int64_t> targetBitrateToNotify;
			};

		public:
			SenderTransportCongestionController(
			  Listener* listener,
			  SharedInterface* shared,
			  SenderTransportCongestionControllerOptions options);

			~SenderTransportCongestionController() override = default;

			SenderTransportCongestionController(const SenderTransportCongestionController&) = delete;
			SenderTransportCongestionController& operator=(const SenderTransportCongestionController&) = delete;

			/**
			 * Tell whether the transport can carry anything at all. Nothing is probed
			 * until it can, so forgetting this leaves the initial probing waiting
			 * forever.
			 */
			void SetNetworkAvailable(bool networkAvailable);

			/**
			 * Take note of a packet that is about to be sent, and write into it the
			 * transport wide sequence number and the abs-send-time that go on the
			 * wire with it.
			 *
			 * @returns The sequence number given to the packet, which has to be given
			 *   back once the packet has actually left, or no value when the packet
			 *   carries no room for it. A packet without that extension cannot be
			 *   reported on, so nothing is taken note of and only its bytes count.
			 */
			std::optional<int64_t> OnRtpPacketToBeSent(
			  RTC::RTP::Packet* packet, const RtpPacketToBeSentOptions& options);

			/**
			 * Feed the confirmation that a packet left through the socket.
			 *
			 * @param sequenceNumber - What `OnRtpPacketToBeSent()` returned for it.
			 * @param size - Length of the packet (bytes), which is what is counted when
			 *   there is no sequence number to look it up by.
			 */
			void OnRtpPacketSent(std::optional<int64_t> sequenceNumber, size_t size, int64_t sentAtUs);

			/**
			 * Feed a received transport wide cc feedback, which is what the delay
			 * based estimation is drawn from.
			 */
			void ReceiveTransportWideCcFeedback(
			  const RTC::RTCP::FeedbackRtpTransportPacket* feedback, int64_t receivedAtUs);

			/**
			 * Feed a received RTCP Receiver Report, which is where the loss of what we
			 * send is measured.
			 */
			void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReportPacket* packet);

			/**
			 * Feed a received REMB, which is what the remote endpoint says it is
			 * willing to receive.
			 */
			void ReceiveEstimatedBitrate(int64_t bitrate);

			/**
			 * Feed the round trip time towards the remote endpoint.
			 */
			void OnRttUpdate(int64_t rttUs);

			/**
			 * Forget a stream, which is what the closing of whatever was sending it
			 * calls for.
			 */
			void RemoveStream(uint32_t ssrc);

			/**
			 * Bound what may be estimated (bps). The bitrate to start from is not
			 * among these, since it only applies before anything has been measured.
			 */
			void SetBitrateLimits(int64_t minBitrate, int64_t maxBitrate);

			/**
			 * Feed the total bitrate that the streams being sent would like to have
			 * (bps), which is what tells whether there is any point in probing higher.
			 */
			void SetDesiredBitrate(int64_t desiredBitrate);

			/**
			 * Size of the overhead that every packet carries below RTP (bytes), which
			 * the link bears along with the packets themselves.
			 */
			void SetPacketOverhead(size_t packetOverhead);

			/**
			 * Latest target bitrate (bps).
			 */
			int64_t GetAvailableBitrate() const;

			/* Pure virtual methods inherited from ProbingScheduler::Listener. */
		public:
			bool OnProbingSchedulerSendRtpPacket(
			  ProbingScheduler* probingScheduler,
			  RTC::RTP::Packet* packet,
			  const Types::ProbeCluster& probeCluster) override;

			/* Pure virtual methods inherited from RTC::TimerHandleInterface::Listener. */
		public:
			void OnTimer(TimerHandleInterface* timer) override;

		private:
			/**
			 * Take note of a packet about to be sent and write its two extensions,
			 * which is the same work for a packet of media and for one of a probe.
			 *
			 * @returns No value when the packet has no room for the transport wide
			 *   sequence number, in which case nothing is taken note of.
			 */
			std::optional<int64_t> AddRtpPacket(
			  RTC::RTP::Packet* packet,
			  const RtpPacketToBeSentOptions& options,
			  std::optional<Types::ProbeCluster> probeCluster);

			/**
			 * Run everything that depends on time having passed rather than on
			 * anything having arrived, which is where the target may rise and where
			 * probes are decided.
			 */
			void Process();

			/**
			 * Take note of the bursts that were asked for, which go out once the call
			 * that asked for them is over.
			 */
			void AddProbeClusters(const std::vector<Types::ProbeClusterConfig>& clusterConfigs);

			/**
			 * Do everything the call leaves pending: hand the bursts to the scheduler
			 * and tell the listener about the target, but only when something it
			 * reasons about actually changed.
			 *
			 * @remarks
			 * - It is the last thing done by every entry point that may leave something
			 *   pending, and the only place where anything of this leaves the class.
			 */
			void ApplyPendingUpdate();

			/**
			 * Reconsider the target and take note of it when the listener has to be
			 * told, which is what every path that may have moved it ends with.
			 */
			void MayNotifyTargetBitrate(int64_t nowUs);

			/**
			 * Bring the bounds into a range the pieces below accept and hand them
			 * down.
			 *
			 * @param applyStartBitrate - Whether the bitrate to start from is handed
			 *   down along with them. It only is the first time, since doing it again
			 *   would force the target back to it and throw away everything the delay
			 *   based path has learnt since.
			 */
			void ResetConstraints(int64_t nowUs, bool applyStartBitrate);

		private:
			// Passed by argument.
			Listener* listener{ nullptr };
			SharedInterface* shared{ nullptr };
			// Allocated by this.
			const std::unique_ptr<TimerHandleInterface> processTimer;
			// Others.
			SendPacketHistory sendPacketHistory;
			FeedbackAdapter feedbackAdapter;
			PacketLossTracker packetLossTracker;
			DelayBasedBwe delayBasedBwe;
			RobustThroughputEstimator acknowledgedBitrateEstimator;
			TargetRateController targetRateController;
			AlrDetector alrDetector;
			ProbeController probeController;
			ProbeBitrateEstimator probeBitrateEstimator;
			ProbingScheduler probingScheduler;
			// What every packet carries below RTP (bytes), which is added to the size
			// the rate control reasons about.
			size_t packetOverhead{ 0 };
			// Bounds in force, already brought into a range the pieces below accept.
			// The one to start from is kept because a change of the other two has to
			// be handed down along with it, and it is raised to the minimum where it
			// is used rather than here, so that it stays what was asked for.
			int64_t minBitrate{ 0 };
			const int64_t startBitrate;
			int64_t maxBitrate{ Types::BitrateInfinite };
			// Whether anything has been sent and whether any transport-cc feedback has
			// arrived, which tell apart a transport that is not being used from one
			// whose remote endpoint does not report.
			bool firstPacketSent{ false };
			bool firstTransportFeedbackReceived{ false };
			// Whether the sender was not filling the link when the latest feedback was
			// handled, so that the instant it stopped being so can be told.
			bool previouslyInAlr{ false };
			// What the listener was last told about, which is what tells whether there
			// is anything new to tell.
			std::optional<int64_t> lastTargetBitrate;
			uint8_t lastFractionLost{ 0 };
			int64_t lastRttUs{ 0 };
			LossBasedController::State lastLossBasedState{ LossBasedController::State::DELAY_BASED_ESTIMATE };
			bool lastIsBandwidthLimited{ true };
			// What the call being served leaves to be done once it is over.
			PendingUpdate pendingUpdate;
		};
	} // namespace BWE
} // namespace RTC

#endif
