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
	static constexpr size_t PacketLossHistogramLength{ 24 };

	/* Instance methods. */

	TransportCongestionControlServer::TransportCongestionControlServer(
	  RTC::TransportCongestionControlServer::Listener* listener,
	  RTC::BweType bweType,
	  size_t maxRtcpPacketLen)
	  : listener(listener), bweType(bweType), maxRtcpPacketLen(maxRtcpPacketLen)
	{
		MS_TRACE();

		switch (this->bweType)
		{
			case RTC::BweType::TRANSPORT_CC:
			{
				// Create a feedback packet.
				this->transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0u, 0u));

				// Set initial packet count.
				this->transportCcFeedbackPacket->SetFeedbackPacketCount(this->transportCcFeedbackPacketCount);

				// Create the feedback send periodic timer.
				this->transportCcFeedbackSendPeriodicTimer = new TimerHandle(this);

				break;
			}

			case RTC::BweType::REMB:
			{
				this->rembServer = new webrtc::RemoteBitrateEstimatorAbsSendTime(this);

				break;
			}
		}
	}

	TransportCongestionControlServer::~TransportCongestionControlServer()
	{
		MS_TRACE();

		delete this->transportCcFeedbackSendPeriodicTimer;
		this->transportCcFeedbackSendPeriodicTimer = nullptr;

		// Delete REMB server.
		delete this->rembServer;
		this->rembServer = nullptr;
	}

	void TransportCongestionControlServer::TransportConnected()
	{
		MS_TRACE();

		switch (this->bweType)
		{
			case RTC::BweType::TRANSPORT_CC:
			{
				this->transportCcFeedbackSendPeriodicTimer->Start(
				  TransportCcFeedbackSendInterval, TransportCcFeedbackSendInterval);

				break;
			}

			default:;
		}
	}

	void TransportCongestionControlServer::TransportDisconnected()
	{
		MS_TRACE();

		switch (this->bweType)
		{
			case RTC::BweType::TRANSPORT_CC:
			{
				this->transportCcFeedbackSendPeriodicTimer->Stop();

				// Create a new feedback packet.
				this->transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(0u, 0u));

				break;
			}

			default:;
		}
	}

	double TransportCongestionControlServer::GetPacketLoss() const
	{
		MS_TRACE();

		return this->packetLoss;
	}

	void TransportCongestionControlServer::IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet)
	{
		MS_TRACE();

		switch (this->bweType)
		{
			case RTC::BweType::TRANSPORT_CC:
			{
				uint16_t wideSeqNumber;

				if (!packet->ReadTransportWideCc01(wideSeqNumber))
				{
					break;
				}

				// Update the RTCP media SSRC of the ongoing Transport-CC Feedback packet.
				this->transportCcFeedbackSenderSsrc = 0u;
				this->transportCcFeedbackMediaSsrc  = packet->GetSsrc();

				this->transportCcFeedbackPacket->SetSenderSsrc(0u);
				this->transportCcFeedbackPacket->SetMediaSsrc(this->transportCcFeedbackMediaSsrc);

				// Provide the feedback packet with the RTP packet info. If it fails,
				// send current feedback and add the packet info to a new one.
				auto result =
				  this->transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs, this->maxRtcpPacketLen);

				switch (result)
				{
					case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::SUCCESS:
					{
						// If the feedback packet is full, send it now.
						if (this->transportCcFeedbackPacket->IsFull())
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

						this->transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs, this->maxRtcpPacketLen);

						break;
					}

					case RTC::RTCP::FeedbackRtpTransportPacket::AddPacketResult::FATAL:
					{
						// Create a new feedback packet.
						this->transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(
						  this->transportCcFeedbackSenderSsrc, this->transportCcFeedbackMediaSsrc));

						// Use current packet count.
						// NOTE: Do not increment it since the previous ongoing feedback
						// packet was not sent.
						this->transportCcFeedbackPacket->SetFeedbackPacketCount(
						  this->transportCcFeedbackPacketCount);

						break;
					}
				}

				MaySendLimitationRembFeedback();

				break;
			}

			case RTC::BweType::REMB:
			{
				uint32_t absSendTime;

				if (!packet->ReadAbsSendTime(absSendTime))
				{
					break;
				}

				// NOTE: nowMs is uint64_t but we need to "convert" it to int64_t before
				// we give it to libwebrtc lib (althought this is implicit in the
				// conversion so it would be converted within the method call).
				auto nowMsInt64 = static_cast<int64_t>(nowMs);

				this->rembServer->IncomingPacket(nowMsInt64, packet->GetPayloadLength(), *packet, absSendTime);

				break;
			}
		}
	}

	void TransportCongestionControlServer::SetMaxIncomingBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		auto previousMaxIncomingBitrate = this->maxIncomingBitrate;

		this->maxIncomingBitrate = bitrate;

		if (previousMaxIncomingBitrate != 0u && this->maxIncomingBitrate == 0u)
		{
			// This is to ensure that we send N REMB packets with bitrate 0 (unlimited).
			this->unlimitedRembCounter = UnlimitedRembNumPackets;

			MaySendLimitationRembFeedback();
		}
	}

	inline void TransportCongestionControlServer::SendTransportCcFeedback()
	{
		MS_TRACE();

		this->transportCcFeedbackPacket->Finish();

		if (!this->transportCcFeedbackPacket->IsSerializable())
		{
			return;
		}

		auto latestWideSeqNumber = this->transportCcFeedbackPacket->GetLatestSequenceNumber();
		auto latestTimestamp     = this->transportCcFeedbackPacket->GetLatestTimestamp();

		// Notify the listener.
		this->listener->OnTransportCongestionControlServerSendRtcpPacket(
		  this, this->transportCcFeedbackPacket.get());

		// Update packet loss history.
		const size_t expectedPackets = this->transportCcFeedbackPacket->GetPacketStatusCount();
		size_t lostPackets           = 0;

		for (const auto& result : this->transportCcFeedbackPacket->GetPacketResults())
		{
			if (!result.received)
			{
				lostPackets += 1;
			}
		}

		if (expectedPackets > 0)
		{
			this->UpdatePacketLoss(static_cast<double>(lostPackets) / expectedPackets);
		}

		// Create a new feedback packet.
		this->transportCcFeedbackPacket.reset(new RTC::RTCP::FeedbackRtpTransportPacket(
		  this->transportCcFeedbackSenderSsrc, this->transportCcFeedbackMediaSsrc));

		// Increment packet count.
		this->transportCcFeedbackPacket->SetFeedbackPacketCount(++this->transportCcFeedbackPacketCount);

		// Pass the latest packet info (if any) as pre base for the new feedback packet.
		if (latestTimestamp > 0u)
		{
			this->transportCcFeedbackPacket->AddPacket(
			  latestWideSeqNumber, latestTimestamp, this->maxRtcpPacketLen);
		}
	}

	inline void TransportCongestionControlServer::MaySendLimitationRembFeedback()
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMs();

		// May fix unlimitedRembCounter.
		if (this->unlimitedRembCounter > 0u && this->maxIncomingBitrate != 0u)
		{
			this->unlimitedRembCounter = 0u;
		}

		// In case this is the first unlimited REMB packet, send it fast.
		// clang-format off
		if (
			(
				(this->bweType != RTC::BweType::REMB && this->maxIncomingBitrate != 0u) ||
				this->unlimitedRembCounter > 0u
			) &&
			(
				nowMs - this->limitationRembSentAtMs > LimitationRembInterval ||
				this->unlimitedRembCounter == UnlimitedRembNumPackets
			)
		)
		// clang-format on
		{
			MS_DEBUG_DEV(
			  "sending limitation RTCP REMB packet [bitrate:%" PRIu32 "]", this->maxIncomingBitrate);

			RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

			packet.SetBitrate(this->maxIncomingBitrate);
			packet.Serialize(RTC::RTCP::Buffer);

			// Notify the listener.
			this->listener->OnTransportCongestionControlServerSendRtcpPacket(this, &packet);

			this->limitationRembSentAtMs = nowMs;

			if (this->unlimitedRembCounter > 0u)
			{
				this->unlimitedRembCounter--;
			}
		}
	}

	void TransportCongestionControlServer::UpdatePacketLoss(double packetLoss)
	{
		// Add the score into the histogram.
		if (this->packetLossHistory.size() == PacketLossHistogramLength)
		{
			this->packetLossHistory.pop_front();
		}

		this->packetLossHistory.push_back(packetLoss);

		// Calculate a weighted average
		size_t weight{ 0 };
		size_t samples{ 0 };
		double totalPacketLoss{ 0 };

		for (auto packetLossEntry : this->packetLossHistory)
		{
			weight++;
			samples += weight;
			totalPacketLoss += weight * packetLossEntry;
		}

		// clang-tidy "thinks" that this can lead to division by zero but we are
		// smarter.
		// NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
		this->packetLoss = totalPacketLoss / samples;
	}

	inline void TransportCongestionControlServer::OnRembServerAvailableBitrate(
	  const webrtc::RemoteBitrateEstimator* /*rembServer*/,
	  const std::vector<uint32_t>& ssrcs,
	  uint32_t availableBitrate)
	{
		MS_TRACE();

		// Limit announced bitrate if requested via API.
		if (this->maxIncomingBitrate != 0u)
		{
			availableBitrate = std::min(availableBitrate, this->maxIncomingBitrate);
		}

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
		this->listener->OnTransportCongestionControlServerSendRtcpPacket(this, &packet);
	}

	inline void TransportCongestionControlServer::OnTimer(TimerHandle* timer)
	{
		MS_TRACE();

		if (timer == this->transportCcFeedbackSendPeriodicTimer)
		{
			SendTransportCcFeedback();
		}
	}
} // namespace RTC
