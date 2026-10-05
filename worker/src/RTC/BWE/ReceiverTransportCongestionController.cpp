#define MS_CLASS "RTC::BWE::ReceiverTransportCongestionController"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ReceiverTransportCongestionController.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		ReceiverTransportCongestionController::ReceiverTransportCongestionController(
		  Listener* listener, SharedInterface* shared, ReceiverTransportCongestionControllerOptions options)
		  : options(options), listener(listener), shared(shared), rembGenerator(this)
		{
			MS_TRACE();

			MS_ASSERT(
			  this->options.congestionControlType == Types::CongestionControlType::TRANSPORT_CC ||
			    this->options.congestionControlType == Types::CongestionControlType::REMB,
			  "no congestion control type given");

			switch (this->options.congestionControlType)
			{
				case Types::CongestionControlType::TRANSPORT_CC:
				{
					this->transportWideCcFeedbackGenerator = std::make_unique<TransportWideCcFeedbackGenerator>(
					  this, this->shared, this->options.maxRtcpPacketLen);

					break;
				}

				case Types::CongestionControlType::REMB:
				{
					this->remoteBitrateEstimator = std::make_unique<RemoteBitrateEstimatorAbsSendTime>(this);

					break;
				}

					NO_DEFAULT();
			}
		}

		void ReceiverTransportCongestionController::ReceiveRtpPacket(
		  int64_t receivedAtUs, const RTC::RTP::Packet* packet, RTC::Media::Kind kind)
		{
			MS_TRACE();

			switch (this->options.congestionControlType)
			{
				case Types::CongestionControlType::TRANSPORT_CC:
				{
					this->transportWideCcFeedbackGenerator->ReceiveRtpPacket(receivedAtUs, packet);

					break;
				}

				case Types::CongestionControlType::REMB:
				{
					// Audio packets take no part in the estimation of the incoming link,
					// so their streams are not announced in the REMB either.
					if (kind == RTC::Media::Kind::AUDIO)
					{
						break;
					}

					this->remoteBitrateEstimator->ReceiveRtpPacket(
					  packet, receivedAtUs, this->shared->GetTimeUs());

					break;
				}

					NO_DEFAULT();
			}

			// The cap has to keep being announced whatever was negotiated, since a
			// remote sender that only reports arrival times is never told anything
			// else.
			this->rembGenerator.MaySendLimitationRembFeedback(this->shared->GetTimeMs());
		}

		void ReceiverTransportCongestionController::OnRttUpdate(int64_t avgRttUs)
		{
			MS_TRACE();

			switch (this->options.congestionControlType)
			{
				case Types::CongestionControlType::TRANSPORT_CC:
				{
					// Nothing is estimated here in this mode, so there is nothing the
					// round trip time bounds.

					break;
				}

				case Types::CongestionControlType::REMB:
				{
					this->remoteBitrateEstimator->OnRttUpdate(avgRttUs);

					break;
				}

					NO_DEFAULT();
			}
		}

		void ReceiverTransportCongestionController::RemoveStream(uint32_t ssrc)
		{
			MS_TRACE();

			switch (this->options.congestionControlType)
			{
				case Types::CongestionControlType::TRANSPORT_CC:
				{
					// No stream is kept track of in this mode.

					break;
				}

				case Types::CongestionControlType::REMB:
				{
					this->remoteBitrateEstimator->RemoveStream(ssrc);

					break;
				}

					NO_DEFAULT();
			}
		}

		void ReceiverTransportCongestionController::SetMaxIncomingBitrate(int64_t bitrate)
		{
			MS_TRACE();

			// NOTE: Zero is how the application asks for no cap at all, while the
			// generator expresses that as no value.
			std::optional<int64_t> maxIncomingBitrate;

			if (bitrate != 0)
			{
				maxIncomingBitrate = bitrate;
			}

			this->rembGenerator.SetMaxIncomingBitrate(this->shared->GetTimeMs(), maxIncomingBitrate);
		}

		std::optional<int64_t> ReceiverTransportCongestionController::GetAvailableBitrate() const
		{
			MS_TRACE();

			switch (this->options.congestionControlType)
			{
				case Types::CongestionControlType::TRANSPORT_CC:
				{
					// The estimating is the remote sender's job in this mode.
					return std::nullopt;
				}

				case Types::CongestionControlType::REMB:
				{
					const int64_t bitrate = this->remoteBitrateEstimator->GetLatestEstimate();

					// NOTE: Zero is how the estimator says that it has nothing, either because
					// no estimation is valid yet or because no stream is active. A measured
					// bitrate never comes out as zero, since the rate control clamps it to the
					// lowest it is configured with.
					if (bitrate == 0)
					{
						return std::nullopt;
					}

					return bitrate;
				}

					NO_DEFAULT();
			}
		}

		void ReceiverTransportCongestionController::OnTransportWideCcFeedbackGeneratorSendPacket(
		  TransportWideCcFeedbackGenerator* /*transportWideCcFeedbackGenerator*/,
		  RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			MS_TRACE();

			this->listener->OnReceiverTransportCongestionControllerSendRtcpPacket(this, packet);
		}

		void ReceiverTransportCongestionController::OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
		  RemoteBitrateEstimatorAbsSendTime* /*remoteBitrateEstimator*/,
		  const std::vector<uint32_t>& ssrcs,
		  int64_t bitrate)
		{
			MS_TRACE();

			this->rembGenerator.OnReceiveBitrateChanged(this->shared->GetTimeMs(), ssrcs, bitrate);
		}

		void ReceiverTransportCongestionController::OnRembGeneratorSendPacket(
		  RembGenerator* /*rembGenerator*/, RTC::RTCP::FeedbackPsRembPacket* packet)
		{
			MS_TRACE();

			this->listener->OnReceiverTransportCongestionControllerSendRtcpPacket(this, packet);
		}
	} // namespace BWE
} // namespace RTC
