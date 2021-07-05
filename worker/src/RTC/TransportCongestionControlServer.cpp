#define MS_CLASS "RTC::TransportCongestionControlServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportCongestionControlServer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream

namespace RTC
{
	/* Static. */

	static constexpr uint64_t TransportCcFeedbackSendInterval{ 100u }; // In ms.
	static constexpr uint64_t LimitationRembInterval{ 1500u };         // In ms.
	static constexpr uint8_t UnlimitedRembNumPackets{ 4u };

	/* Instance methods. */

	TransportCongestionControlServer::TransportCongestionControlServer(
	  RTC::TransportCongestionControlServer::Listener* listener,
	  RTC::BweType bweType,
	  size_t maxRtcpPacketLen)
	  : controller(TransportCongestionControlServer::Strategy::Create(
	      this, listener, bweType, maxRtcpPacketLen))
	{
		MS_TRACE();
	}

	TransportCongestionControlServer::~TransportCongestionControlServer()
	{
		MS_TRACE();
	}

	void TransportCongestionControlServer::TransportConnected()
	{
		MS_TRACE();
    return controller->TransportConnected();
	}

	void TransportCongestionControlServer::TransportDisconnected()
	{
		MS_TRACE();
		return controller->TransportDisconnected();
	}

	void TransportCongestionControlServer::IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet)
	{
		MS_TRACE();
		return controller->IncomingPacket(nowMs, packet);
	}

  void TransportCongestionControlServer::SetMaxIncomingBitrate(uint32_t bitrate)
  {
    MS_TRACE();
    return controller->SetMaxIncomingBitrate(bitrate);
  }

	void TransportCongestionControlServer::Strategy::MaySendLimitationRembFeedback()
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMs();

		// May fix unlimitedRembCounter.
		if (unlimitedRembCounter > 0u && maxIncomingBitrate != 0u)
			unlimitedRembCounter = 0u;

		// In case this is the first unlimited REMB packet, send it fast.
		// clang-format off
		if (NeedSendREMB() &&
        (nowMs - limitationRembSentAtMs > LimitationRembInterval
          || unlimitedRembCounter == UnlimitedRembNumPackets))
		// clang-format on
		{
			MS_DEBUG_DEV(
			  "sending limitation RTCP REMB packet [bitrate:%" PRIu32 "]", maxIncomingBitrate);

			RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

			packet.SetBitrate(maxIncomingBitrate);
			packet.Serialize(RTC::RTCP::Buffer);

			// Notify the listener.
      listener->OnTransportCongestionControlServerSendRtcpPacket(parent, &packet);

			limitationRembSentAtMs = nowMs;

			if (unlimitedRembCounter > 0u)
				unlimitedRembCounter--;
		}
	}

  void TransportCongestionControlServer::Strategy::SetMaxIncomingBitrate(uint32_t bitrate)
  {
    MS_TRACE();

    auto previousMaxIncomingBitrate = maxIncomingBitrate;

    maxIncomingBitrate = bitrate;

    if (previousMaxIncomingBitrate != 0u && maxIncomingBitrate == 0u)
    {
      // This is to ensure that we send N REMB packets with bitrate 0 (unlimited).
      unlimitedRembCounter = UnlimitedRembNumPackets;

      MaySendLimitationRembFeedback();
    }
  }

  struct TransportCongestionControlStrategyTWCC
	  : public TransportCongestionControlServer::Strategy
	  , public Timer::Listener
	{
    TransportCongestionControlStrategyTWCC() {
      // Create a feedback packet.
      transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0u, 0u));

      // Set initial packet count.
      transportCcFeedbackPacket->SetFeedbackPacketCount(this->transportCcFeedbackPacketCount);

      // Create the feedback send periodic timer.
      transportCcFeedbackSendPeriodicTimer = new Timer(this);
    }
    ~TransportCongestionControlStrategyTWCC() {
      delete transportCcFeedbackSendPeriodicTimer;
      transportCcFeedbackSendPeriodicTimer = nullptr;
    }

    bool NeedSendREMB() const override {
      return (maxIncomingBitrate != 0u || unlimitedRembCounter > 0u);
    }

    void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet) override
    {
      uint16_t wideSeqNumber;

      if (!packet->ReadTransportWideCc01(wideSeqNumber))
        return;

      // Update the RTCP media SSRC of the ongoing Transport-CC Feedback packet.
      transportCcFeedbackSenderSsrc = 0u;
      transportCcFeedbackMediaSsrc  = packet->GetSsrc();

      transportCcFeedbackPacket->SetSenderSsrc(0u);
      transportCcFeedbackPacket->SetMediaSsrc(packet->GetSsrc());

      // Provide the feedback packet with the RTP packet info. If it fails,
      // send current feedback and add the packet info to a new one.
      auto result =
        transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs, this->maxRtcpPacketLen);

      switch (result)
      {
        case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::SUCCESS:
        {
          // If the feedback packet is full, send it now.
          if (transportCcFeedbackPacket->IsFull())
          {
            MS_DEBUG_DEV("transport-cc feedback packet is full, sending feedback now");

            SendTransportCcFeedback();
          }

          break;
        }

        case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::MAX_SIZE_EXCEEDED:
        {
          // Send ongoing feedback packet and add the new packet info to the
          // regenerated one.
          SendTransportCcFeedback();

          transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs, this->maxRtcpPacketLen);

          break;
        }

        case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::FATAL:
        {
          // Create a new feedback packet.
          transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(
            transportCcFeedbackSenderSsrc, transportCcFeedbackMediaSsrc));

          // Use current packet count.
          // NOTE: Do not increment it since the previous ongoing feedback
          // packet was not sent.
          transportCcFeedbackPacket->SetFeedbackPacketCount(
            transportCcFeedbackPacketCount);

          break;
        }
      }

      MaySendLimitationRembFeedback();
    }

    void OnTimer(Timer* timer) override {
      if (timer == transportCcFeedbackSendPeriodicTimer)
        SendTransportCcFeedback();
		}

		void TransportConnected() override {
      transportCcFeedbackSendPeriodicTimer->Start(
        TransportCcFeedbackSendInterval, TransportCcFeedbackSendInterval);
		}

		void TransportDisconnected() override {
      this->transportCcFeedbackSendPeriodicTimer->Stop();

      // Create a new feedback packet.
      this->transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0u, 0u));
    }

    void SendTransportCcFeedback()
    {
      MS_TRACE();

      if (!transportCcFeedbackPacket->IsSerializable())
        return;

      auto latestWideSeqNumber = transportCcFeedbackPacket->GetLatestSequenceNumber();
      auto latestTimestamp     = transportCcFeedbackPacket->GetLatestTimestamp();

      // Notify the listener.
      listener->OnTransportCongestionControlServerSendRtcpPacket(
        parent, transportCcFeedbackPacket.get());

      // Create a new feedback packet.
      transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(
        transportCcFeedbackSenderSsrc, transportCcFeedbackMediaSsrc));

      // Increment packet count.
      transportCcFeedbackPacket->SetFeedbackPacketCount(++transportCcFeedbackPacketCount);

      // Pass the latest packet info (if any) as pre base for the new feedback packet.
      if (latestTimestamp > 0u)
      {
        transportCcFeedbackPacket->AddPacket(
          latestWideSeqNumber, latestTimestamp, maxRtcpPacketLen);
      }
    }

    Timer* transportCcFeedbackSendPeriodicTimer{ nullptr };

    std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket> transportCcFeedbackPacket;

    // size_t maxRtcpPacketLen{ 0u };
    uint32_t transportCcFeedbackSenderSsrc{ 0u };
    uint32_t transportCcFeedbackMediaSsrc{ 0u };

    uint8_t transportCcFeedbackPacketCount{ 0u };
  };

	struct TransportCongestionControlStrategyREMB
    : public TransportCongestionControlServer::Strategy
	  , public webrtc::RemoteBitrateEstimator::Listener
	{
    TransportCongestionControlStrategyREMB() {
      rembServer = new webrtc::RemoteBitrateEstimatorAbsSendTime(this);
    }
    ~TransportCongestionControlStrategyREMB() {
      delete rembServer;
      rembServer = nullptr;
    }

    bool NeedSendREMB() const override {
      return unlimitedRembCounter > 0u;
    }

    uint32_t GetAvailableBitrate() override {
      return rembServer->GetAvailableBitrate();
    }

    void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet) override {
      uint32_t absSendTime;

      if (!packet->ReadAbsSendTime(absSendTime))
        return;

      // NOTE: nowMs is uint64_t but we need to "convert" it to int64_t before
      // we give it to libwebrtc lib (althought this is implicit in the
      // conversion so it would be converted within the method call).
      auto nowMsInt64 = static_cast<int64_t>(nowMs);

      rembServer->IncomingPacket(nowMsInt64, packet->GetPayloadLength(), *packet, absSendTime);
    }

		void OnRembServerAvailableBitrate(
		  const webrtc::RemoteBitrateEstimator* /*rembServer*/,
      const std::vector<uint32_t>& ssrcs,
      uint32_t availableBitrate) override {
      // Limit announced bitrate if requested via API.
      if (this->maxIncomingBitrate != 0u)
        availableBitrate = std::min(availableBitrate, this->maxIncomingBitrate);

#if MS_LOG_DEV_LEVEL == 3
      std::ostringstream ssrcsStream;

		if (!ssrcs.empty())
		{
			std::copy(ssrcs.begin(), ssrcs.end() - 1, std::ostream_iterator<uint32_t>(ssrcsStream, ","));
			ssrcsStream << ssrcs.back();
		}

		MS_DEBUG_DEV(
		  "sending RTCP REMB packet [bitrate:%" PRIu32 ", ssrcs:%s]",
		  availableBitrate,
		  ssrcsStream.str().c_str());
#endif

      RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

      packet.SetBitrate(availableBitrate);
      packet.SetSsrcs(ssrcs);
      packet.Serialize(RTC::RTCP::Buffer);

      // Notify the listener.
      listener->OnTransportCongestionControlServerSendRtcpPacket(parent, &packet);
    }

    webrtc::RemoteBitrateEstimatorAbsSendTime* rembServer{ nullptr };
  };

	std::unique_ptr<TransportCongestionControlServer::Strategy> TransportCongestionControlServer::Strategy::Create(
	  TransportCongestionControlServer* parent,
	  TransportCongestionControlServer::Listener* listener,
	  BweType bweType,
	  size_t maxRtcpPacketLen)
	{
    Strategy* c = nullptr;
    switch (bweType) {
      case BweType::TRANSPORT_CC:
        c = new TransportCongestionControlStrategyTWCC();
				break;
      case BweType::REMB:
        c = new TransportCongestionControlStrategyREMB();
				break;
      default:
        return nullptr;
    }
    c->parent = parent;
		c->listener = listener;
		c->maxRtcpPacketLen = maxRtcpPacketLen;

		return std::unique_ptr<Strategy>(c);
  }

} // namespace RTC
