#ifndef MS_RTC_SHM_CONSUMER_HPP
#define MS_RTC_SHM_CONSUMER_HPP

#include "json.hpp"
#include "Lively.hpp"
#include "DepLibSfuShm.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SeqManager.hpp"
#include "RTC/RateCalculator.hpp"
#include "handles/Timer.hpp"

using json = nlohmann::json;

namespace RTC
{
	class RtpLostPktRateCounter
	{
	public:
		explicit RtpLostPktRateCounter(size_t windowSize=2500, float scale=90000.0f) 
			: lostPackets(windowSize, scale), totalPackets(windowSize, scale)
		{
		}

	public:
		void Update(RTC::RtpPacket* packet);
		uint64_t GetLossRate(uint64_t nowMs)
		{
			return this->lostPackets.GetRate(nowMs);
		}
		uint64_t GetTotalRate(uint64_t nowMs)
		{
			return this->totalPackets.GetRate(nowMs);
		}

	private:
		RateCalculator      lostPackets;  // lost packets (calculated by gaps in seqIds)
		RateCalculator      totalPackets; // total packets (calculated by seqIds)
		size_t lost{ 0u };
		size_t total{ 0u };

		uint64_t firstSeqId{ 0u };
		uint64_t lastSeqId{ 0u };
	};

	class ShmConsumer : public RTC::Consumer,
											public RTC::RtpStreamSend::Listener,
											public DepLibSfuShm::ShmCtx::Listener,
											public Timer::Listener
	{
	public:
		ShmConsumer(const std::string& id, const std::string& producerId, RTC::Consumer::Listener* listener, json& data, DepLibSfuShm::ShmCtx *shmCtx);
		~ShmConsumer() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void FillJsonScore(json& jsonObject) const override;
		void HandleRequest(Channel::ChannelRequest* request) override;
		bool IsActive() const override;
		void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void ProducerRtcpSenderReport(RTC::RtpStream* rtpStream, bool first) override;
		uint8_t GetBitratePriority() const override;
		uint32_t IncreaseLayer(uint32_t bitrate, bool considerLoss) override;
		void ApplyLayers() override;
		uint32_t GetDesiredBitrate() const override;

		void SendRtpPacket(RTC::RtpPacket* packet) override;
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, RTC::RtpStreamSend* rtpStream, uint64_t now) override;
		std::vector<RTC::RtpStreamSend*> GetRtpStreams() override;
		void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) override;
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket) override;
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType, uint32_t ssrc) override;
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report) override;
		uint32_t GetTransmissionRate(uint64_t now) override;
		float GetRtt() const override;
		uint32_t GetBitrate(uint64_t nowMs);

	private:
		void UserOnTransportConnected() override;
		void UserOnTransportDisconnected() override;
		void UserOnPaused() override;
		void UserOnResumed() override;
		void CreateRtpStream();
		void RequestKeyFrame();
		void FillShmWriterStats(json& jsonObject) const;

		void WritePacketToShm(RTC::RtpPacket* packet);
		bool VideoOrientationChanged(RTC::RtpPacket* packet);

		// NACK testing: if testNackEachMs > 0 then drop a pkt each ms and form a NACK request
		bool TestNACK(RTC::RtpPacket* packet);
		uint64_t lastNACKTestTs {0};
		uint64_t testNackEachMs {0};

		/* Pure virtual methods inherited from RtpStreamSend::Listener. */
	public:
		void OnRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore) override;
		void OnRtpStreamRetransmitRtpPacket(RTC::RtpStreamSend* rtpStream, RTC::RtpPacket* packet) override;

	/* Pure virtual methods inherited from DepLibSfuShm::ShmCtx::Listener. */
	public:
		void OnNeedToSync() override;

	/* Pure virtual methods inherited from Timer. */
	protected:
		void OnTimer(Timer* timer) override;

	private:
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		// Others.
		std::vector<RTC::RtpStreamSend*> rtpStreams;
		RTC::RtpStream* producerRtpStream{ nullptr };
		bool keyFrameSupported{ false };
		bool syncRequired{ false };
		RTC::SeqManager<uint16_t> rtpSeqManager;

		DepLibSfuShm::ShmCtx       *shmCtx{ nullptr };         // Handle to shm context which will be received from ShmTransport during transport.consume()
		uint16_t                   rotation{ 0 };             // Current rotation value for video read from RTP packet's videoOrientationExtensionId
		bool                       rotationDetected{ false }; // Whether video rotation data was ever picked in this stream, then we only write it into shm if there was a change
		RTC::RtpDataCounter        shmWriterCounter;          // Use to collect and report shm writing stats, for RTP only (RTCP is not handled by ShmConsumer) TODO: move into ShmCtx
		RTC::RtpLostPktRateCounter lostPktRateCounter;

	private:
		void   OnIdleShmConsumer();          // Call from OnTimer() to notify nodejs Consumer
		Timer* shmIdleCheckTimer{ nullptr }; // Check for incoming RTP packets, declare idle after 20 seconds
		bool 	 idle{ false };                // Idle if inactivityCheckTime is not reset within 20 seconds
	public:
		Lively::AppData appData;
	};

	/* Inline methods. */

	inline bool ShmConsumer::IsActive() const
	{
		return (RTC::Consumer::IsActive() && this->producerRtpStream);
	}

	inline std::vector<RTC::RtpStreamSend*> ShmConsumer::GetRtpStreams()
	{
		return this->rtpStreams;
	}

	/* Copied from RtpStreamSend */
	inline uint32_t ShmConsumer::GetBitrate(uint64_t nowMs)
	{
		return this->shmWriterCounter.GetBitrate(nowMs);
	}
} // namespace RTC

#endif
