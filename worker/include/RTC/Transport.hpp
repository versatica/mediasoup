#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "json.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/TransportTuple.hpp"
#include "handles/Timer.hpp"
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Producer;
	class Consumer;

	class Transport : public RTC::Producer::Listener,
	                  public RTC::Consumer::Listener,
	                  public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportClosed(RTC::Transport* transport) = 0;
		};

	public:
		// RTP header extension ids that must be shared by all the Producers using
		// the same Transport.
		// NOTE: These ids are the original ids in the RTP packet (before the Producer
		// maps them to the corresponding ids in the Router).
		struct HeaderExtensionIds
		{
			uint8_t absSendTime{ 0 }; // 0 means no abs-send-time id.
			uint8_t mid{ 0 };         // 0 means no MID id.
			uint8_t rid{ 0 };         // 0 means no RID id.
		};

	public:
		Transport(uint32_t id, Listener* listener);
		virtual ~Transport();

	public:
		void Close();
		void FillJson(json& jsonObject) const      = 0;
		void FillJsonStats(json& jsonObject) const = 0;
		void SetNewProducerIdFromRequest(Channel::Request* request, std::string& producerId) const;
		RTC::Producer* GetProducerFromRequest(Channel::Request* request) const;
		void SetNewConsumerIdFromRequest(Channel::Request* request, std::string& consumerId) const;
		RTC::Consumer* GetConsumerFromRequest(Channel::Request* request) const;
		void HandleProducer(RTC::Producer* producer);
		void HandleConsumer(RTC::Consumer* consumer);
		virtual void SendRtpPacket(RTC::RtpPacket* packet)     = 0;
		virtual void SendRtcpPacket(RTC::RTCP::Packet* packet) = 0;

	protected:
		void HandleRtcpPacket(RTC::RTCP::Packet* packet);
		void ReceiveRtcpRemb(RTC::RTCP::FeedbackPsRembPacket* remb);

	private:
		virtual bool IsConnected() const = 0;
		void SendRtcp(uint64_t now);
		virtual void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) = 0;
		RTC::Consumer* GetConsumer(uint32_t ssrc) const;

		/* Pure virtual methods inherited from RTC::ProducerListener. */
	public:
		void OnProducerClosed(RTC::Producer* producer) override;
		void OnProducerPaused(RTC::Producer* producer) override;
		void OnProducerResumed(RTC::Producer* producer) override;
		void OnProducerRtpPacket(
		  RTC::Producer* producer,
		  RTC::RtpPacket* packet,
		  RTC::RtpEncodingParameters::Profile profile) override;
		void OnProducerProfileEnabled(
		  RTC::Producer* producer,
		  RTC::RtpEncodingParameters::Profile profile,
		  const RTC::RtpStream* rtpStream) override;
		void OnProducerProfileDisabled(
		  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile) override;

		/* Pure virtual methods inherited from RTC::ConsumerListener. */
	public:
		void OnConsumerClosed(RTC::Consumer* consumer) override;
		void OnConsumerKeyFrameRequired(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t id{ 0 };

	protected:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		Timer* rtcpTimer{ nullptr };
		std::unordered_map<std::string, RTC::Producer*> producers;
		std::unordered_map<std::string, RTC::Consumer*> consumers;
		// Others.
		bool closed{ false };
		// Others (RtpListener).
		RtpListener rtpListener;
		struct HeaderExtensionIds headerExtensionIds;
		// Others (REMB).
		std::tuple<uint64_t, std::vector<uint32_t>> recvRemb;
	};
} // namespace RTC

#endif
