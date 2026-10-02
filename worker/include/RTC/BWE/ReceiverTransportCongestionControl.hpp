#ifndef MS_RTC_BWE_RECEIVER_TRANSPORT_CONGESTION_CONTROL_HPP
#define MS_RTC_BWE_RECEIVER_TRANSPORT_CONGESTION_CONTROL_HPP

#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/RembGenerator.hpp"
#include "RTC/BWE/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/BWE/TransportWideCcFeedbackGenerator.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTP/Packet.hpp"
#include "SharedInterface.hpp"
#include <vector>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Everything the receiving side of a transport does about congestion, which
		 * amounts to telling the remote sender how much it may send towards us.
		 *
		 * How that is said depends on what both endpoints negotiated. With
		 * transport-cc nothing is estimated here: the instant each packet arrived is
		 * reported back and the remote sender does the estimating. With REMB it is
		 * the other way round, and a bitrate is computed here and announced.
		 *
		 * The cap the application sets is announced as a REMB either way, since that
		 * is the only way to tell a remote sender to hold back.
		 */
		class ReceiverTransportCongestionControl : public TransportWideCcFeedbackGenerator::Listener,
		                                           public RemoteBitrateEstimatorAbsSendTime::Listener,
		                                           public RembGenerator::Listener
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * An RTCP packet is ready for the remote sender.
				 *
				 * @param packet - Packet to send. It belongs to the caller, which
				 *   destroys it once this call returns, so it must be neither kept nor
				 *   deleted here.
				 */
				virtual void OnReceiverTransportCongestionControlSendRtcpPacket(
				  ReceiverTransportCongestionControl* receiverTransportCongestionControl,
				  RTC::RTCP::Packet* packet) = 0;
			};

			struct ReceiverTransportCongestionControlOptions
			{
				/**
				 * Mechanism both endpoints negotiated, which decides what is done with
				 * every packet that comes in.
				 */
				Types::CongestionControlType congestionControlType;
				/**
				 * Largest an RTCP packet may be on this transport (bytes).
				 */
				size_t maxRtcpPacketLen{ RTC::Consts::RtcpPacketMaxSize };
			};

		public:
			ReceiverTransportCongestionControl(
			  Listener* listener, SharedInterface* shared, ReceiverTransportCongestionControlOptions options);

			~ReceiverTransportCongestionControl() override = default;

			ReceiverTransportCongestionControl(const ReceiverTransportCongestionControl&) = delete;
			ReceiverTransportCongestionControl& operator=(const ReceiverTransportCongestionControl&) = delete;

			/**
			 * Feed a received RTP packet.
			 *
			 * @param receivedAtUs - Instant the packet arrived.
			 */
			void ReceiveRtpPacket(int64_t receivedAtUs, const RTC::RTP::Packet* packet);

			/**
			 * Cap what the remote sender is told it may send (bps), or zero to let it
			 * send whatever the incoming link is estimated to bear.
			 */
			void SetMaxIncomingBitrate(int64_t bitrate);

			/**
			 * Latest estimation of the incoming link (bps), or no value while there is
			 * none and whenever the estimating is the remote sender's job.
			 */
			std::optional<int64_t> GetAvailableBitrate() const;

			/* Pure virtual methods inherited from TransportWideCcFeedbackGenerator::Listener. */
		public:
			void OnTransportWideCcFeedbackGeneratorSendPacket(
			  TransportWideCcFeedbackGenerator* transportWideCcFeedbackGenerator,
			  RTC::RTCP::FeedbackRtpTransportPacket* packet) override;

			/* Pure virtual methods inherited from RemoteBitrateEstimatorAbsSendTime::Listener. */
		public:
			void OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
			  RemoteBitrateEstimatorAbsSendTime* remoteBitrateEstimator,
			  const std::vector<uint32_t>& ssrcs,
			  int64_t bitrate) override;

			/* Pure virtual methods inherited from RembGenerator::Listener. */
		public:
			void OnRembGeneratorSendPacket(
			  RembGenerator* rembGenerator, RTC::RTCP::FeedbackPsRembPacket* packet) override;

		private:
			const ReceiverTransportCongestionControlOptions options;

			// Passed by argument.
			Listener* listener{ nullptr };
			SharedInterface* shared{ nullptr };
			// Allocated by this, the first one only in transport-cc mode and the
			// second one only in REMB mode.
			std::unique_ptr<TransportWideCcFeedbackGenerator> transportWideCcFeedbackGenerator;
			std::unique_ptr<RemoteBitrateEstimatorAbsSendTime> remoteBitrateEstimator;
			// Others.
			// It lives in both modes, since the cap is announced as a REMB whatever
			// was negotiated.
			RembGenerator rembGenerator;
		};
	} // namespace BWE
} // namespace RTC

#endif
