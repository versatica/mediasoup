#define MS_CLASS "RTC::BWE::ReceiverTransportCongestionControl"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/BWE/ReceiverTransportCongestionControl.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace BWE
	{
		/* Instance methods. */

		ReceiverTransportCongestionControl::ReceiverTransportCongestionControl(
		  Listener* listener, SharedInterface* shared, ReceiverTransportCongestionControlOptions options)
		  : options(options), listener(listener), shared(shared), rembGenerator(this)
		{
			MS_TRACE();

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

		void ReceiverTransportCongestionControl::ReceiveRtpPacket(
		  int64_t receivedAtUs, const RTC::RTP::Packet* packet)
		{
			MS_TRACE();

			if (this->transportWideCcFeedbackGenerator)
			{
				this->transportWideCcFeedbackGenerator->IncomingPacket(receivedAtUs, packet);
			}

			if (this->remoteBitrateEstimator)
			{
				// NOTE: The packet is being fed as it arrives, so the instant it arrived
				// at is also the current one.
				this->remoteBitrateEstimator->IncomingPacket(packet, receivedAtUs, receivedAtUs);
			}

			// The cap has to keep being announced whatever was negotiated, since a
			// remote sender that only reports arrival times is never told anything
			// else.
			this->rembGenerator.MaySendLimitationRembFeedback(Utils::Time::TimeUsToMs(receivedAtUs));
		}

		void ReceiverTransportCongestionControl::SetMaxIncomingBitrate(int64_t bitrate)
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

		std::optional<int64_t> ReceiverTransportCongestionControl::GetAvailableBitrate() const
		{
			MS_TRACE();

			if (!this->remoteBitrateEstimator)
			{
				return std::nullopt;
			}

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

		void ReceiverTransportCongestionControl::OnTransportWideCcFeedbackGeneratorSendRtcpPacket(
		  TransportWideCcFeedbackGenerator* /*transportWideCcFeedbackGenerator*/,
		  RTC::RTCP::FeedbackRtpTransportPacket* packet)
		{
			MS_TRACE();

			this->listener->OnReceiverTransportCongestionControlSendRtcpPacket(this, packet);
		}

		void ReceiverTransportCongestionControl::OnRemoteBitrateEstimatorAbsSendTimeBitrateChanged(
		  RemoteBitrateEstimatorAbsSendTime* /*remoteBitrateEstimator*/,
		  const std::vector<uint32_t>& ssrcs,
		  int64_t bitrate)
		{
			MS_TRACE();

			this->rembGenerator.OnReceiveBitrateChanged(this->shared->GetTimeMs(), ssrcs, bitrate);
		}

		void ReceiverTransportCongestionControl::OnRembGeneratorSendRemb(
		  RembGenerator* /*rembGenerator*/, RTC::RTCP::FeedbackPsRembPacket* packet)
		{
			MS_TRACE();

			this->listener->OnReceiverTransportCongestionControlSendRtcpPacket(this, packet);
		}
	} // namespace BWE
} // namespace RTC
