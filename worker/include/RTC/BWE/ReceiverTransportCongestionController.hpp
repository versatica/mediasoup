#ifndef MS_RTC_BWE_RECEIVER_TRANSPORT_CONGESTION_CONTROLLER_HPP
#define MS_RTC_BWE_RECEIVER_TRANSPORT_CONGESTION_CONTROLLER_HPP

#include "common.hpp"
#include "RTC/BWE/RembGenerator.hpp"
#include "RTC/BWE/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/BWE/TransportWideCcFeedbackGenerator.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
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
		 * How that is said is decided packet by packet, by what each one carries,
		 * since a transport may well have one stream negotiated with transport-cc
		 * and another with only abs-send-time. A packet with a transport wide
		 * sequence number has its arrival time reported back and the remote sender
		 * does the estimating; one with abs-send-time instead is estimated here and
		 * the result announced as a REMB; one with neither says nothing to anybody.
		 *
		 * The cap the application sets is announced as a REMB either way, since that
		 * is the only way to tell a remote sender to hold back.
		 */
		class ReceiverTransportCongestionController : public TransportWideCcFeedbackGenerator::Listener,
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
				 * @param packet - Packet to send. It belongs to the caller, so the
				 *   listener must neither destroy it nor keep it around after this call
				 *   returns.
				 */
				virtual void OnReceiverTransportCongestionControllerSendRtcpPacket(
				  ReceiverTransportCongestionController* receiverTransportCongestionControl,
				  RTC::RTCP::Packet* packet) = 0;
			};

			struct ReceiverTransportCongestionControllerOptions
			{
				/**
				 * Largest an RTCP packet may be on this transport (bytes).
				 */
				size_t maxRtcpPacketLen{ RTC::Consts::RtcpPacketMaxSize };
			};

		public:
			ReceiverTransportCongestionController(
			  Listener* listener,
			  SharedInterface* shared,
			  ReceiverTransportCongestionControllerOptions options);

			~ReceiverTransportCongestionController() override = default;

			ReceiverTransportCongestionController(const ReceiverTransportCongestionController&) = delete;
			ReceiverTransportCongestionController& operator=(
			  const ReceiverTransportCongestionController&) = delete;

			/**
			 * Feed a received RTP packet.
			 *
			 * @param kind - Media the stream the packet belongs to carries, or no
			 *   value if the packet belongs to no stream of ours, which happens to
			 *   the packets still in flight when a Producer is closed.
			 * @param receivedAtUs - Instant the packet arrived.
			 *
			 * @remarks
			 * - Such a packet still has to be fed, since leaving a hole in the
			 *   transport-cc feedback tells the remote sender that the link lost it.
			 */
			void ReceiveRtpPacket(
			  const RTC::RTP::Packet* packet, std::optional<RTC::Media::Kind> kind, int64_t receivedAtUs);

			/**
			 * Feed the round trip time towards the remote senders, which bounds how
			 * often the estimation of the incoming link may be reduced.
			 */
			void OnRttUpdate(int64_t avgRttUs);

			/**
			 * Forget a stream, so that it no longer takes part in the estimation of
			 * the incoming link nor in the streams the remote sender is told about.
			 */
			void RemoveStream(uint32_t ssrc);

			/**
			 * Cap what the remote sender is told it may send (bps), or zero to let it
			 * send whatever the incoming link is estimated to bear.
			 */
			void SetMaxIncomingBitrate(int64_t bitrate);

			/**
			 * Latest estimation of the incoming link (bps), or no value while there is
			 * none.
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
			// Passed by argument.
			Listener* listener{ nullptr };
			SharedInterface* shared{ nullptr };
			TransportWideCcFeedbackGenerator transportWideCcFeedbackGenerator;
			RemoteBitrateEstimatorAbsSendTime remoteBitrateEstimator;
			RembGenerator rembGenerator;
		};
	} // namespace BWE
} // namespace RTC

#endif
